/********************************************************************************************
   PROGRAMA PRINCIPAL DO SMWF ( Smart Meter Water Flux) - OFICINAS 4.0 - Versão 1.4
   Instituição: Instituto Federal de Alagoas - Campus Palmeira dos índios

   Descrição: Ele irá fazer a leitura de eletrodos no fluído
   e irá enviar a vazão para um servidor WEB. Além disso, também receberá dados
   via Bluetooth e terá opções de auto-teste para avaliar seu funcionamento.

   Ultima atualização: 03/12/2021 (SAULO JOSÉ ALMEIDA SILVA)
 ********************************************************************************************/


/**************************************************************************************************|
  |     NOME       |     CORE       |    PRIORIDADE     |               DESCRIÇÃO                  |
  |----------------|----------------|-------------------|------------------------------------------|
  |-vTaskDataSensor|      00        |         03        |    Leitura/Tratamento e envio dos dados  |
  |                |                |                   |  do sensor de ref e o protótipo.         |
  |----------------|----------------|-------------------|------------------------------------------|
  |-vTaskDisplay   |      01        |         01        |   Imprime dados no display OLED          |
  |----------------|----------------|-------------------|------------------------------------------|
  |-vTaskMYSQL     |      01        |         02        |   Envia dados do protótipo para o MYSQL  |
  |----------------|----------------|-------------------|------------------------------------------|
  |-vTaskWiFiReset |      00        |         04        | Chama a rotina de reconfigurar WiFi      |
  |----------------|----------------|-------------------|------------------------------------------|
  |-vTaskBobinas   |      00        |         01        | Efetua operações de transição nas bobinas|
****************************************************************************************************



    >> NÚCLEOS:
        => APP_CPU_NUM = 00
        => PRO_CPU_NUM = 01

    >> Fórmula utilizada no Protótipo:
     V = U.(pi.D)/(4.k.B)

     V: Vazão dentro do tubo (m^3/s);
     U: Tensão nos eletrodos (V)
     pi: 3.1415926535...;
     D: Diâmetro do tubo; (m)
     k: Constante encontrada nos testes;
     B: Módulo do campo gerado pelas bobinas; (T)

    BUG ATUAL:
    - Ao mantar a interrupção, o código nem inicia. ( )
    - Task do Reset WiFi está com problema de sintexe. ( )

    NECESSIDADE:
    - Corrigir problema na tarefa de resetar WiFi pela interrupção ();

  ==================================|| INCLUDES DAS BIBLIOTECAS || ===============================*/

//Bibliotecas para utilizar o display OLED
#include "Arduino.h"
#include "heltec.h"

//Biblioteca de controle PWM
#include "driver/mcpwm.h"

//Header do protótipo.
#include "Prototipo_def.h"
#include "imagem.h"

//Bibliotecas para comuniacação via WiFi
#include <WiFi.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SPI.h>

/*================================|| SE E SENÃO ||==============================================*/
//Obs: Necessário para rodas as funções em mais de um núcleo..
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif
/*================================|| Handle das tasks e protótipos ||===========================*/

//Filas:
//xFilaDisplay: Fila para enviar dados para o Display
//xFilaMYSQL: Fila para enviar dados para o MySQL
QueueHandle_t xFilaDisplay;
QueueHandle_t xFilaMySQL;

//Tasks
TaskHandle_t vTaskDataSensorHandle;
TaskHandle_t vTaskDisplayHandle;
TaskHandle_t vTaskMySQLHandle;
TaskHandle_t vTaskWiFiResetHandle;
TaskHandle_t vTaskBobinasHandle;

//Semaforos Handles
SemaphoreHandle_t WiFiResetSemaphore;

//Protótipo das Tasks
void vTaskDataSensor(void* pvParamaters);
void vTaskDisplay(void* pvParamaters);
void vTaskMySQL(void* pvParamaters);
void vTaskWiFiReset(void* pvParamaters);
void vTaskBobinas(void* pvParamaters);

//Função para callback da configuração AcessPoint
void configModeCallback(WiFiManager *myWiFiManager);

//Função de callback para quando salvar as informações
void saveConfigCallback();

//Variáveis globais para amostragem (OBS: PODE MODIFICAR SE NÃO FUNCIONAR)
volatile int data_sensor = 0;
volatile unsigned long timer1 = 0;

//Timer para configurar o tempo de envio da informação no MySQL.
volatile unsigned long timer2 = 0;
/*=============================|| Função da ISR ||=============================================*/
//Função de callback da ISR do WiFi.
void IRAM_ATTR WiFiISRCallback()
{
  //Variável para analisar a task de maior prioridade
  BaseType_t xHighPriorityTask = pdTRUE;

  //Variáveis para analisar o tempo de acionamento do botão.
  static unsigned long lastTime = 0;
  unsigned long newTime = millis();

  //Analisa o tempo de deboucing do botão para ISR
  if (newTime - lastTime < 50) {}
  else
  {
    //Libera o semaforo pela interrupção
    xSemaphoreGiveFromISR(WiFiResetSemaphore, &xHighPriorityTask);

    //Realiza a troca de contexto, fazendo a tarefa com maior prioridade funcionar.
    if (xHighPriorityTask == pdTRUE)
    {
      portYIELD_FROM_ISR(); //Troca para a tarefa com maior prioridade
    }
  }
}
/*======================================|| SETUP ||=============================================*/
void setup() {
  //Iniciando Serial
  Serial.begin(115200);

  //Gerando interrupções
  //attachInterrupt(digitalPinToInterrupt(pin_ISR_WiFi), WiFiISRCallback, RISING);

  //Funções para inicializar os equipamentos.
  Serial.println("[SMWF]: Configurando sistema...");
  initDisplay(); //Inicializa Display OLED
  initSaidas(); //Configura saídas
  initWiFi(); //Inicia comunicação WiFi  initFreeRTOS(); //Inicia funções do FreeRTOS
  initPWMbobinas(); //Configura controle das bobinas;
  initFreeRTOS(); //Inicia Tarefas.
  Serial.println("[SMWF]: Configurações finalizadas!\n\n");
}
/*======================================|| loop ||==============================================*/
void loop() {
  vTaskDelete(NULL); //Deletando a task loop.
  //Serial.println(analogRead(PIN_PROTOTIPO));
  //delay(500);
}
/*=====================================|| DEFINIÇÃO DAS FUNÇÕES ||==============================*/
//Função para iniciar tasks e filas do FreeRTOS.
void initFreeRTOS()
{
  Serial.println("[SMWF]: Iniciando FreeRTOS.");
  BaseType_t returnSensor, returnDisplay, returnMySQL, returnWiFiReset, returnBobina;

  //Gerando filas
  xFilaDisplay = xQueueCreate(
                   1, //Quantidade de elementos
                   sizeof(sensorRef));//Tamanho de cada item.

  //Fila do MySQL
  xFilaMySQL = xQueueCreate(1, sizeof(sensorRef));

  //Gerando semaforo binário para o botão de resetar WiFi
  WiFiResetSemaphore = xSemaphoreCreateBinary();

  if (xFilaMySQL != NULL && xFilaDisplay != NULL)
  {
    //Gerando Tasks
    returnSensor = xTaskCreatePinnedToCore(
                     vTaskDataSensor //Nome da tarefa
                     , "TaskSensor" //Nome para Debug
                     , configMINIMAL_STACK_SIZE + 1024 //Tamanho da memória utilizada pela tarefa.
                     , NULL // Parâmetro que a tarefa irá receber.
                     , 3 //Prioridade da tarefa
                     , &vTaskDataSensorHandle //Objeto para manipular tarefa
                     , 0); //Utilizará o primeiro núcleo de processamento
    if (returnSensor != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1. Não foi possível gerar a tarefa do sensor.");
      errorMessage(666, 5000);
      while (1);
    }

    returnDisplay = xTaskCreatePinnedToCore(
                      vTaskDisplay //Tarefa que irá somar
                      , "TaskDisplay" //Nome para Debug
                      , configMINIMAL_STACK_SIZE + 1024 //Tamanho da memória reservado
                      , NULL //Parâmetro inicial
                      , 2 //Prioridade da tarefa
                      , &vTaskDisplayHandle //Handle  da tarefa
                      , 1); //Utilizará o segundo núcleo de processamento.

    if (returnDisplay != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1. Não foi possível gerar a tarefa do Display.");
      errorMessage(666, 5000);
      while (1);
    }
    returnMySQL = xTaskCreatePinnedToCore(
                    vTaskMySQL //Tarefa
                    , "TaskMySQL" //Nome para Debug
                    , configMINIMAL_STACK_SIZE + 1024 //Memória reservada
                    , NULL //Parâmetro inícial
                    , 2 //Prioridade
                    , &vTaskMySQLHandle //Handle da tarefa
                    , 1); //Utilizará o segundo núcleo de processamento.


    if (returnMySQL != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1.  Não foi possível gerar a tarefa do MySQL.");
      errorMessage(666, 5000);
      while (1);
    }

    returnWiFiReset = xTaskCreatePinnedToCore(
                        vTaskWiFiReset //Tarefa
                        , "TaskWiFiReset" //Nome para Debug
                        , configMINIMAL_STACK_SIZE + 1024 //Memória reservada
                        , NULL //Parâmetro inicial enviado
                        , 4 //Prioridade
                        , &vTaskWiFiResetHandle //Handle da tarefa
                        , 0); //Utilizará o primeiro núcleo de processamento.


    if (returnWiFiReset != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1.  Não foi possível gerar a tarefa do WiFiReset.");
      errorMessage(666, 5000);
      while (1);
    }

    returnBobina = xTaskCreatePinnedToCore(
                     vTaskBobinas //Tarefa
                     , "Controle Bobinas" //Nome para debug
                     , configMINIMAL_STACK_SIZE + 1024 //Memória utilziada
                     , NULL // Variável de inicialização
                     , 1 //Prioridade
                     , &vTaskBobinasHandle // Handle da tarefa
                     , 0); //Utiliza o primeiro núcleo de processamento


    if (returnBobina != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1.  Não foi possível gerar a tarefa do vTaskBobina..");
      errorMessage(666, 5000);
      while (1);
    }
    Serial.println("[SMWF]: FreeRTOS iniciado com sucesso.");


  }

  else
  {
    Serial.println("[SMWF]: Não foi possível gerar as Tasks... Houve um erro!");
    errorMessage(666, 5000);
    while (1);
  }
}

/*IMPLEMENTAÇÃO DAS TAREFAS*/
//Tarefa de coletar dados
void vTaskDataSensor(void* pvParamaters)
{
  (void*) pvParamaters;
  //Estrutura para enviar os dados
  struct sensorRef sensorValue;

  //Fazendo tratamento dos dados
  //float dados[max_int], acc, media;

  while (1)
  {
    /*
      acc = 0;
      //Limpa todos os dados anteriores.
      for (int i = 0; i < max_int; i++)
      {
      dados[i] = readPrototipo();
      //Serial.println("Valor do dados [" + Streing(i) + "]:" + String(dados[i]));
      acc += dados[i];
      }
      //Serial.println("Valor acumulado:" + String(acc));
      media = acc / max_int; //Média dos valores tomados.
    */

    //Capturando dados.
    data_sensor = readPrototipo(); //Captura dados de tensão na entrada analógica;
    samplingTime(); // Função que analisa se o tempo de amostragem passou e chama a função do filtro de média móvel, atualizando o buffer circular

    //Seção de debug:
    //Serial.println("[DATA_t]: Leitura: " + String(data_sensor) + "// MovingAvarege:" + String(movingAverage(0)));

    //Salvando dados na struct
    sensorValue.pino = PIN_PROTOTIPO;
    sensorValue.valor = transUnit(movingAverage(0));


    //Seção de envio de dados.
    /* O tempo de delay entre o display e o envio da MySQL é de 30 s.*/
    xQueueSend(xFilaDisplay, &sensorValue, portMAX_DELAY);

    //If para contar 30 segundos.
    if (millis() - timer2 > time_mysql) {

      xQueueSend(xFilaMySQL, &sensorValue, portMAX_DELAY);
      timer2 = millis();
    }

    vTaskDelay(3);
  }
}

/*
  Obs: Os dados da função de filtro de média móvel precisam ser atualizados por alguma tarefa, então
  é necessário que a tarefa do MySQL  ocorra em um tempo fixo.
*/

//Tarefa de imprimir no display
void vTaskDisplay(void* pvParamaters)
{
  (void) pvParamaters;
  struct sensorRef sensorValue;
  while (1)
  {
    if (xQueueReceive(xFilaDisplay, &sensorValue, portMAX_DELAY) == pdTRUE)
    {
      //Imprimindo valor no display.
      printDisplay(sensorValue.valor);
    }
    vTaskDelay(3);
  }
}

//Tarefa do MySQL
void vTaskMySQL(void* pvParamaters)
{
  (void) pvParamaters;
  while (1)
  {
    sensorRef sensorValue;
    if (xQueueReceive(xFilaMySQL, &sensorValue, portMAX_DELAY) == pdTRUE)
    {
      //Ele mesmo pega o IP do host pelo gateway, sem a necessidade de colocalo.
      IPAddress host = WiFi.gatewayIP();

      //Conectando ao webcliente.
      Serial.print("[MySQL_T]: Conectando com ");
      Serial.println(host);

      //Criando objeto WiFiCliente, para guardar dados.
      WiFiClient client;

      const int httpPort = 80;

      if (!client.connect(host, httpPort))
      {
        //Suspende as demais tarefas para evitar problemas.
        Serial.println("[MySQL_T]: Falha na conexão. Verifique o Host ou a Porta. Verifique se o XAMPP está ligado!");
        Serial.println("[MySQL_T]: Outras tarefas foram pausadas para evitar erros de processamento.");
        vTaskSuspend(vTaskDataSensorHandle);
        vTaskSuspend(vTaskDisplayHandle);

        errorMessage(404, 5000);

        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP.restart();
      }

      //Pedindo o url para salvar dados
      String url = "http://localhost/nodemcu/salvar.php?";
      url += "vazao=";
      url += sensorValue.valor;
      url += "&id_medidor=";
      url +=  IP_SENSOR;


      Serial.print("[MySQL_T]: Requisitando URL:");
      Serial.println(url);

      //Faz a solicitação de enviar dados
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n\r\n");

      //Verifica tempo de conexão
      unsigned long timeout = millis();

      //Verifica se o cliente está respondendo a requisição
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println("[MySQL_T]: >>> Client Timeout !");
          client.stop();
          errorMessage(425, 5000);
          vTaskDelay(pdMS_TO_TICKS(10000));
          ESP.restart();
        }
      }

      //O cliente responde, então analisa o que ele está respondendo.
      while (client.available()) {
        //Lê os dados do cliente até encontrar o '\r' e salva em uma instring
        String line = client.readStringUntil('\r');
        //Serial.print(line);

        //Compara a mensagem deixada no cliente com o valor line
        if (line.indexOf("salvo_com_sucesso_tabela_2") != -1)
        {
          Serial.println("[MySQL_T]: Arquivo foi salvo com sucesso.");
        }
        else if (line.indexOf("erro_ao_salvar_tabela_2") != -1)
        {
          Serial.println("[MySQL_T]: Houve um erro ao salvar o dado. Verifique as configurações.");
          //Poderia ligar um led para me avisar que não está funcionando.
          errorMessage(401, 5000);
        }
      }
    }
    Serial.println("[MySQL_T]:Fechando conexão...\n");
    vTaskDelay(pdMS_TO_TICKS(10000));//Espera 1 min até cadastrar um novo dado de vazão
  }
}


//Tarefa para resetar o WiFi (Precisa de semáforo!)
void vTaskWiFiReset(void* pvParamaters)
{
  (void) pvParamaters;

  while (1)
  {
    //Espera liberar o semaforo binário para liberar a operação de resetar WiFi.
    if (xSemaphoreTake(WiFiResetSemaphore, portMAX_DELAY) == pdTRUE)
    {
      //Suspendendo as demais tasks para evitar problemas de acesso e deixar apenas essa tarefa funcionando.
      vTaskSuspend(vTaskDataSensorHandle);
      vTaskSuspend(vTaskDisplayHandle);
      vTaskSuspend(vTaskMySQLHandle);


      Serial.println("[ISR_WIFI]: Suspendendo as demais tarefas para dar prioridade a esta.");

      //Após pegar o semáforo, ele libera essa parte.
      //Preciso modificar toda essa parte. Está confusa.
      WiFiManager wifiManager;

      if (!wifiManager.autoConnect("SMWF_AP", "12345678"))
      {
        Serial.println("[ISR_WIFI]: Falha ao reconectar.");
        delay(2000);
        ESP.restart();
      }

      //Retorna as tarefas.
      vTaskResume(vTaskDataSensorHandle);
      vTaskResume(vTaskDisplayHandle);
      vTaskResume(vTaskMySQLHandle);

    }
  }
}

//Tarefa de controle das bobinas.
void vTaskBobinas(void* pvParamaters)
{
  while (1)
  {
    /* Bobina oscilando
      //O código aqui é de exemplo, vai ser modificado mediante as necessidades do projeto.
      bobina_ascendente(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);//Gira o motor no sentido horário
      vTaskDelay(pdMS_TO_TICKS(3000));

      bobina_descendente(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);
      vTaskDelay(pdMS_TO_TICKS(3000));
    */

    //Bobina parada no sentido ascendente
    bobina_ascendente(MCPWM_UNIT_0, MCPWM_TIMER_0, 100);

    vTaskDelay(5);
  }
}


/* Definição das demais funções*/
//Procedimentos
void initDisplay()
{
  Serial.println("[SMWF]: Inicializando o Display...");
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Sebrial Enable*/);
  Heltec.display->init(); // Inicio o display
  Heltec.display->flipScreenVertically(); //Seto na vertical
  Heltec.display->setFont(ArialMT_Plain_10); //Indico o tipo de letra que irá utilizar.

  creditsProto();
  delay(3000);
  Serial.println("[SMWF]: Display iniciado com sucesso.");
}

//Função para imprimir dados no display
void printDisplay(double protoData)
{
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(30, 0, "<SMWF>");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 20, ">Vazão: " + String(protoData) + " L/min");
  Heltec.display->drawString(0, 30, ">WiFi: " + String(WiFi.SSID()));
  Heltec.display->drawString(0, 40, ">Bobina:");

  Heltec.display->display();
}

//Procedimento de iniciar GPIOs
void initSaidas()
{
  Serial.println("[SMWF]: Iniciando configuração das GPIOs");

  //Configurando Pinos
  pinMode(PIN_PROTOTIPO, INPUT);
  pinMode(pin_ISR_WiFi, INPUT);


  //Debug
  Serial.println("[InitSaidas]: Pinos =>");
  Serial.println(">>Pin_Protótipo: " + String(PIN_PROTOTIPO));
  Serial.println(">>ISR_WiFi: " + String(pin_ISR_WiFi));
  Serial.println(">>IP_Sensor: " + String(IP_SENSOR));


  //CHamando carregamento.
  initBarDisplay(0, 25, CharS);
  initBarDisplay(25, 50, CharI);
  initBarDisplay(50, 75, CharW);
  initBarDisplay(75, 100, CharF);
  delay(500);

  //Serial print para comunicar finalização da tarefa.
  Serial.println("[SMWF]:Procedimento finalizado.");
}

//Iniciando o WiFi do Lora 32
void initWiFi()
{
  Serial.println("[SMWF]: Iniciando o WiFi...");
  //Gerando objeto wifiManager
  WiFiManager wifiManager;

  //Reseta as configurações do WiFiManager... Posso tirar, caso eu queria.
  //wifiManager.resetSettings();

  //Funções de callback
  wifiManager.setAPCallback(configModeCallback);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.autoConnect("SMWF_AP", "12345678");
}


//Callback para WiFiManager
void configModeCallback(WiFiManager * myWiFiManager)
{
  //  Serial.println("Entered config mode");
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede

  //Imprime dados no display.
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(10, 0, "WiFi Manager");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 20, "AP: " + String(ssidWM));
  Heltec.display->drawString(0, 35, "Senha: " + String(senhaWM));
  Heltec.display->drawString(0, 50, "IP: " + String("192.168.4.1"));
  Heltec.display->display();
}

//Callback para quando o WiFi for salvo
void saveConfigCallback()
{
  Serial.println("[SMWF]: Configuração salva");
  Heltec.display->clear();
  Heltec.display->drawXbm(32, 0, Certo_width, Certo_height, Certo_bits);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(34, 54, "WiFi Salvo!");
  Heltec.display->display();
}

/*Funções de retorno*/
//Função para ler dados do protótipo
float readPrototipo()
{
  double value;

  /*
    O valor do sensor é 1/5 do valor real. Quando o sensor é alimentado com 3.3 V, ele terá uma leitura de 0
    a 16,5 V.

    Vout = Vin/5;

    Como a tensão associada é com 12 bits de resolução. A tensão mínima captada pelo ESP32 é de;

    Resolução = 3,3 V /2^12 ~= 0.8057 mV

    Como o valor de entrada é 5 vezes maior, então é preciso de uma tensão mínima de :
    Vmin = Vmin(sensor)*5 = 0.0857 mV * 5 = 4,0285 mV

    O ESP transforma dados de 0 a 3,3 V em valores analógicos de 0 a 4095.

    3.3 V ==> 4095
    x V => y

    x = 3.3 Y /4095

    Agora, sabendo que x é o Vout do sensor, então:

    Vreal = 5*Vout = 5*3.3*Sinal/4095;

    V(real)=Sinal.16,5/(4095)

    Obs: Esse valor dá um valor próximo do real. É preciso ajustar esse valor para dar o real.
  */

  //Retorna valor da tensão.
  value = analogRead(PIN_PROTOTIPO) * 16.5 / 4095;

  return value;
}


//Função para transformar de tensão para litros.
double transUnit(float protoData)
{
  return protoData * (MAT_PI * DIAM_TUBE) / (4 * CONST_SENSOR * CAMPO_BOBINA);
}

//Função para calcular a diferença entre dois valores.
float difValues(float valueA, float valueB)
{
  return 100 * (valueB - valueA) / valueB;
}

//Função inicial do projeto, para mostrar logotipo
void creditsProto()
{
  Heltec.display->clear();
  Heltec.display->drawXbm(32, 0, SMWF_width, SMWF_height, SMWF_bits);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(50, 54, "SMWF");
  Heltec.display->display();
}

//Barra de carregamento para indicar inicialização dos processos.
void initBarDisplay(int iniLoad, int finLoad, char* infChar)
{
  for (int i = iniLoad; i <= finLoad; i = i + 5)
  {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawProgressBar(0, 40, 120, 10, i);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(60, 10, "INICIALIZANDO");
    Heltec.display->drawString(64, 25, String(i) + "%");
    Heltec.display->drawString(60, 54, infChar);
    Heltec.display->display();
    delay(100);
  }
}

//Exibir mensagem de erro no display.
void errorMessage(int erroTxt, uint8_t TimeDelay)
{
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawXbm(32, 0, erro_width, erro_height, erro_bits);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(37, 54, "Error " + String(erroTxt));
  Heltec.display->display();
  delay(TimeDelay);
}

//Inicia as configurações do controle PWM
void initPWMbobinas()
{
  //Serial.println("[SMWF]: Iniciando automação PWM");
  //mcpwm_gpio_init(unidade PWM 0, saida A, porta GPIO) => Instancia o MCPWMA para a porta GPIO_PWM0A_OUT
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);

  //mcpwm_gpio_init(unidade PWM 0, saida b, porta GPIO) => Instancia o MCPWMA para a porta GPIO_PWM0B_OUT
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);

  //Preparando variável com configuração para PWM
  mcpwm_config_t pwm_config;

  pwm_config.frequency = 1000; //frequência = 1000 hz
  pwm_config.cmpr_a = 0; //ciclo de trabalho (duty cycle) do PWMxA =0 de 0 a 100%
  pwm_config.cmpr_b = 0; //Ciclo de trabalho (duty cycle) do PWMxB=0
  pwm_config.counter_mode = MCPWM_UP_COUNTER; // Para MCPWM assimétrico
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // Define ciclo de trabalho em nível alto

  //Inicia(Unidade 0, Timer 0, Configuração PWM)
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
  //Serial.println("[SMWF]: Finalizando automação PWM");
}

static void bobina_ascendente(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cicle)
{
  //mcpwm_set_signal_low(unidade PWM(O ou 1), número do timer (0,1 ou 2), saída (A ou B));
  mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);//Desliga o sinal do MCPWM no Operaor B (Define o sinal em Baixo)

  //mcpwm_set_duty(unidade PWM(0 ou 1), Número do timer (0, 1 ou 2), Saída (A ou B),  Ciclo de trabalho (% do PWM));
  mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cicle);//Configura a porcentagem do PWM no Operador A (Ciclo de trabalho)

  //mcpwm_set_duty_type(unidade PWM(0 ou 1), Número do Timer (0,1 ou 2), Operador (A ou B), Nível do ciclo de trabalho));
  mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);//define o ciclo de trabalho (alto ou baixo)

  //Nota: chame essa função toda vez que for chamado "mcpwm_set_signal_low" ou "mcpwm_set_signal_high"
  //para manter o ciclo de trabalho configurado anteriormente.
}
static void bobina_descendente(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cicle)
{
  //mcpwm_set_signal_low(unidade PWM(O ou 1), número do timer (0,1 ou 2), saída (A ou B));
  mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);//Desliga o sinal do MCPWM no Operaor B (Define o sinal em Baixo)

  //mcpwm_set_duty(unidade PWM(0 ou 1), Número do timer (0, 1 ou 2), Saída (A ou B),  Ciclo de trabalho (% do PWM));
  mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cicle);//Configura a porcentagem do PWM no Operador A (Ciclo de trabalho)

  //mcpwm_set_duty_type(
  mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);//define o ciclo de trabalho (alto ou baixo)

  //Nota: chame essa função toda vez que for chamado "mcpwm_set_signal_low" ou "mcpwm_set_signal_high"
  //para manter o ciclo de trabalho configurado anteriormente.
}
static void bobina_desligada(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
  mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);//Desliga o sinal no operador A
  mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);//Desliga o sinal no operador B
}

/*======================================|| FUNÇÕES DE LEITURA ||==================================================*/
//OBS: Propenso a bug
//Função de média móvel
float movingAverage(bool update_output) { // A função de media movel trabalha com variavel estática, que salva as variaveis sem perder
  static  int last_reads[max_int]; // Esse é o vetor que servirá como buffer circular
  static  int Position = 0; // A posicao atual de leitura, que deverá ficar salva
  static long Sum = 0; // A soma total do buffer circular
  static float average = 0; // A media, que é a saída da função quando é chamada
  static bool reset_vector = 1;  // A variavel para saber se é a primeira execução. Se for, ele zera todo o buffer circular.

  if (reset_vector) { // Zerando todo o buffer circular, para que as subtrações das sobrescrição não atrapalhe o filtro
    for (int i = 0; i <= max_int; i++) {
      last_reads[i] = 0;
    }
    reset_vector = 0;
    //    Serial.println("Não entra mais no laço");
  }
  if (update_output == 0) return ((double)average); // Se o parametro recebido na funcao for zero, ele retorna somente o valor de media calculado anteriormente

  else { // Caso seja 1, signfica que está no tempo de amostragem, e ai atualiza a variável média
    Sum = data_sensor - last_reads[Position % max_int] + Sum;
    last_reads[Position % max_int] = data_sensor;
    average = (float)Sum / (float)(max_int);
    Position = (Position + 1) % max_int;
    return ((double)average);
  }
}

//Função para verificar tempo de amostragem.
void samplingTime() { // Essa função verifica se o tempo de amostragem  selecionado ocorreu
  if (millis() - timer1 > delay_int) { // Caso o tempo de amostragem tenha ocorrido, ele envia 1 para a função de filtro de media movel
    //Dessa forma a função sabe que é para atualizar o valor de saída para um novo valor filtrado
    movingAverage(1);
    timer1 = millis(); // atualiza para contar o tempo mais uma vez
  }
}


