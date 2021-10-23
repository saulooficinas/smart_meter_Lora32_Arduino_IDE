/********************************************************************************************
   PROGRAMA PRINCIPAL DO SMWF ( Smart Meter Water Flux) - OFICINAS 4.0 - Versão 1.1
   Instituição: Instituto Federal de Alagoas - Campus Palmeira dos índios

   Descrição: Ele irá fazer a leitura de eletrodos no fluído
   e irá enviar a vazão para um servidor WEB. Além disso, também receberá dados
   via Bluetooth e terá opções de auto-teste para avaliar seu funcionamento.

   Ultima atualização: 21/10/2021 (SAULO JOSÉ ALMEIDA SILVA)
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
****************************************************************************************************

    >> NÚCLEOS:
        => APP_CPU_NUM = 00 = ARDUINO_RUNNING_CORE
        => PRO_CPU_NUM = 01 = ARDUINO_RUNNING_CORE

    >> Fórmula utilizada no Protótipo:
     V = U.(pi.D)/(4.k.B)

     V: Vazão dentro do tubo (m^3/s);
     U: Tensão nos eletrodos (V)
     pi: 3.1415926535...;
     D: Diâmetro do tubo; (m)
     k: Constante encontrada nos testes;
     B: Módulo do campo gerado pelas bobinas; (T)


      Bug: OLED NÃO INICIA COM O FREERTOS.
      Provavel causa: tarefas executam muito rápido e os dados não chegam no OLED.

  ==================================|| INCLUDES DAS BIBLIOTECAS || ===============================*/

//Bibliotecas para utilizar o display OLED
#include "Arduino.h"
#include "heltec.h"

//Header do protótipo.
#include "Prototipo_def.h"
#include "imagem.h"

//Bibliotecas para comuniacação via WiFi
#include <WiFi.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <WebServer.h>

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

//Protótipo das Tasks
void vTaskDataSensor(void* pvParamaters);
void vTaskDisplay(void* pvParamaters);
void vTaskMySQL(void* pvParamaters);
void vTaskWiFiReset(void* pvParamaters);

//Função para callback da configuração AcessPoint
void configModeCallback(WiFiManager *myWiFiManager);

//Função de callback para quando salvar as informações
void saveConfigCallback();
/*======================================|| SETUP ||=============================================*/
void setup() {
  //Iniciando Serial
  Serial.begin(115200);


  //Gerando interrupções


  //Funções para inicializar os equipamentos.
  Serial.println("[SMWF]: Configurando sistema...");
  initDisplay(); //Inicializa Display OLED
  initSaidas(); //Configura saídas
  initWiFi(); //Inicia comunicação WiFi
  initFreeRTOS(); //Inicia funções do FreeRTOS
  Serial.println("[SMWF]: Configurações finalizadas!\n\n");
}
/*======================================|| loop ||==============================================*/
void loop() {
  digitalWrite(13, !digitalRead(13));
}
/*=====================================|| DEFINIÇÃO DAS FUNÇÕES ||==============================*/
//Função para iniciar tasks e filas do FreeRTOS.
void initFreeRTOS()
{
  Serial.println("[SMWF]: Iniciando FreeRTOS.");
  BaseType_t returnSensor, returnDisplay, returnMySQL, returnWiFiReset;

  //Gerando filas
  xFilaDisplay = xQueueCreate(
                   1, //Quantidade de elementos
                   sizeof(sensorRef));//Tamanho de cada item.

  xFilaMySQL = xQueueCreate(1, sizeof(sensorRef));

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
      while (1);
    }

    returnDisplay = xTaskCreatePinnedToCore(
                      vTaskDisplay //Tarefa que irá somar
                      , "TaskDisplay" //Nome para Debug
                      , configMINIMAL_STACK_SIZE + 1024 //Tamanho da memória reservado
                      , NULL //Parâmetro inicial
                      , 1 //Prioridade da tarefa
                      , &vTaskDisplayHandle //Handle  da tarefa
                      , 1); //Utilizará o segundo núcleo de processamento.

    if (returnDisplay != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1. Não foi possível gerar a tarefa do Display.");
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
      while (1);
    }

    returnWiFiReset = xTaskCreatePinnedToCore(
                        vTaskWiFiReset //Tarefa
                        , "TaskWiFiReset" //Nome para Debug
                        , configMINIMAL_STACK_SIZE + 1024 //Memória reservada
                        , NULL //Parâmetro inicial enviado
                        , 2 //Prioridade
                        , &vTaskWiFiResetHandle //Handle da tarefa
                        , 1); //Utilizará o segundo núcleo de processamento.
    if (returnWiFiReset != pdTRUE)
    {
      Serial.println("[SMWF]: Erro 1.  Não foi possível gerar a tarefa do WiFiReset.");
      while (1);
    }
    Serial.println("[SMWF]: FreeRTOS iniciado com sucesso.");
  }
  else
  {
    Serial.println("[SMWF]: Não foi possível gerar os dados... Houve um erro!");
    while (1);
  }
}

/*IMPLEMENTAÇÃO DAS TAREFAS*/
//Tarefa de coletar dados
void vTaskDataSensor(void* pvParamaters)
{
  (void) pvParamaters;

  while (1)
  {
    //Estrutura para enviar os dados
    struct sensorRef sensorValue;

    //Fazendo tratamento dos dados
    static float dados[max_int];

    float acc = 0, media;

    for (int i = 0; i < max_int; i++)
    {
      dados[i] = readPrototipo();
      acc += dados[i];
    }

    media = acc / max_int; //Média dos valores tomados.

    //Salvando dados na estrutura
    sensorValue.pino[0] = PIN_REF;
    sensorValue.valor[0] = readRefSensor();

    sensorValue.pino[1] = PIN_PROTOTIPO;
    sensorValue.valor[1] = transUnit(media);
    Serial.println("[SENSOR_T]: Dados do Ref e do Protótipo: " + String(sensorValue.valor[0]) + " " + String(sensorValue.valor[1]));

    //Enviado dados para a task de MySQL
    xQueueSend(xFilaMySQL, &sensorValue, portMAX_DELAY);
    vTaskDelay(2);

    //Enviado dados para a task do Display
    xQueueSend(xFilaDisplay, &sensorValue, portMAX_DELAY);
    vTaskDelay(4);
  }
}

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
      printDisplay(sensorValue.valor[0], sensorValue.valor[1]);
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
      //Conectando ao webcliente.
      Serial.print("[MySQL_T]: Conectando com: ");
      Serial.println(host);

      //Criando objeto WiFiCliente, para guardar dados.
      WiFiClient client;

      const int httpPort = 80;

      if (!client.connect(host, httpPort))
      {
        Serial.println("[MySQL_T]: Falha na conexão");
        return;
      }

      //Pedindo o url para salvar dados
      String url = "http://localhost/nodemcu/salvar.php?";
      url += "vazao=";
      url += sensorValue.valor[1];
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
      while (client.available() == 0) {
        if (millis() - timeout > 5000) {
          Serial.println("[MySQL_T]: >>> Client Timeout !");
          client.stop();
          return;
        }
      }

      while (client.available()) {
        String line = client.readStringUntil('\r');
        //Serial.print(line);

        if (line.indexOf("salvo_com_sucesso") != -1)
        {
          Serial.println();
          Serial.println("[MySQL_T]: Arquivo foi salvo com sucesso.");
        }
        else if (line.indexOf("erro_ao_salvar") != -1)
        {
          Serial.println();
          Serial.println("[MySQL_T]: Houve um erro ao salvar o dado. Verifique as configurações.");
          //Poderia ligar um led para me avisar que não está funcionando.
        }
      }

      Serial.println();
      Serial.println("[MySQL_T]:Fechando conexão...");
      xQueueSend(xFilaDisplay, &sensorValue, portMAX_DELAY);
      vTaskDelay(pdMS_TO_TICKS(10000));//Espera 1 min até cadastrar um novo dado de vazão
    }
  }
}

//Tarefa para resetar o WiFi (Precisa de semáforo)
void vTaskWiFiReset(void* pvParamaters)
{
  (void) pvParamaters;

  while (1)
  {
    vTaskDelete(NULL);
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

void printDisplay(float a, float b)
{
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->drawString(30, 0, "<SMWF>");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 20, ">>Vazão: " + String(a) + " L/min");
  Heltec.display->drawString(0, 30, ">>Ref: " + String(b) + " L/min");
  Heltec.display->drawString(0, 40, ">>Rede WiFi: " + String("Almeida/Aquino"));
  Heltec.display->display();
}

//Procedimento de iniciar GPIOs
void initSaidas()
{
  Serial.println("[SMWF]: Iniciando configuração das GPIOs");

  //Configurando Pinos
  pinMode(PIN_PROTOTIPO, INPUT);
  pinMode(PIN_REF, INPUT);

  //Debug
  Serial.println("[InitSaidas]: Pinos =>");
  Serial.println(">>Pin_Protótipo: " + String(PIN_PROTOTIPO));
  Serial.println(">>Pin_Ref: " + String(PIN_REF));
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
  wifiManager.resetSettings();

  //Funções de callback
  wifiManager.setAPCallback(configModeCallback);

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.autoConnect("SWMF_AP", "12345678");
}

void configModeCallback(WiFiManager *myWiFiManager)
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
  float value;

  //Obs: Seria 25/1023.0 caso fosse um Arduino
  value = analogRead(PIN_PROTOTIPO) * (3.3 * 3.3) / (1023.0);

  return value;
}

//Função para saber o valor da leitura do sensor
float readRefSensor()
{
  return 0;
}

//Função para transformar de tensão para litros.
float transUnit(float d)
{
  return d * (MAT_PI * DIAM_TUBE) / (4 * CONST_SENSOR * CAMPO_BOBINA);
}

//Função para calcular a diferença entre dois valores.
float difValues(float a, float b)
{
  return 100 * (b - a) / b;
}

void creditsProto()
{
  Heltec.display->clear();
  Heltec.display->drawXbm(32, 0, SMWF_width, SMWF_height, SMWF_bits);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(50, 54, "SMWF");
  Heltec.display->display();
}

void initBarDisplay(int a, int b,   char* v)
{
  for (int i = a; i <= b; i = i + 5)
  {
    Heltec.display->clear();
    Heltec.display->drawProgressBar(0, 40, 120, 10, i);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(60, 10, "INICIALIZANDO");
    Heltec.display->drawString(64, 25, String(i) + "%");
    Heltec.display->drawString(60, 54, v);
    Heltec.display->display();
    delay(100);
  }
}
