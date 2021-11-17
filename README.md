# Projeto SMWF (Smart Meter Water Flux) em LoRa 32

Versão 1.2 do Projeto das Oficinas 4.0, voltado para a Área do IOT e Eletricidade.

Organizado por alunos do Instituto Federal de Alagoas - Campus Palmeida dos índios (PIN);

*OBS: Foi utilizado o Arduino IDE, devido a facilidade com as bibliotecas presentes nele.*

## Objetivos do projeto:

* Display OLED;

* Comunicação com MySQL;

* Conexão WiFi com WiFiManager;

* WebServer;

* Medidor de vazão eletromagnético;

* APP para vizualizar dados;

O código para a comunicação com o MySQL é via script em PHP utilizando um [WebServer mantido pelo XAMPP](https://github.com/saulooficinas/conexaoWebServer).

## Materiais utilizados

## Bibliotecas utilizadas no projeto
- [WiFi Manager](https://drive.google.com/file/d/1SWHUtYhCIDw9M1s6ZrNVSJ-hdCXBlVad/view);
- Heltec WiFi Esp 32 LoRa;
- WebServer (Presente no Arduino IDE)
- DNSS (Presente no Arduino IDE)

Obs: A instalação da biblioteca da Heltec foi a partir do video do [Fernado K](https://www.fernandok.com/2019/03/instalacao-do-esp32-lora-na-arduino-ide.html);

## Definição das funções utilizadas
### 1)  Procedimentos: 
   1.1) **void initDisplay()**
 
Procedimento utilizado para fazer as configurações do display do Lora.
     
  
  
  
   1.2) **void initFreeRTOS()**

Procedimento utilizado para configurar as tarefas, filas e samaforos do FreeRTOS.
     
 
 
 
   1.3) **void printDisplay(float ProtoData, float RefData)**

Procedimento utilizado para imprimir os dados de vazão no display.
     
**- Parâmetros:**
  - **ProtoData:** variável float para vazão vinda do protótipo;
  - **RefData:** variável float utilizara para a vazão vinda do sensor de referência.
   
   
   
   
1.4) **void initSaidas()**

Procedimento utilizado para configurar pinos do microcontrolador, iniciar comunicação serial e ISRS.
     
     
     
     
1.5) **void initWiFi()**
     
Procedimento utilizado para iniciar comunicação WiFi do microcontrolador. Neste projeto, foi utilizado o WiFi Manager para facilitar essa configuração.
     
     
     
     
1.6) **void creditsProto()**

Procedimento utilizado para mostrar a logotipo do projeto no Display, além de informações opicionais.
    
    
    
1.7) **void initBarDisplay(int iniLoad, int finLoad, char infChar)**

Procedimento utilizado para mostar uma barra de carreegamento no display, que pode ser utilizada para mostrar o andamento de alguma tarefa e etc.
    
**- Parâmetros:**
   - **iniLoad:** Valor inicial de carregamento da barra; 
   - **finLoad:** Valor final do carregamento da barra;
   - **infChar:** String com texto utilizada para dar informação de qual função está sendo desenvolvida.
    

1.8) **void errorMessage(uint8_t erroTxt, uint8_t TimeDelay)**
    
Procedimento utilizado para mostrar mensagem de erro. Tem como principal função ajudar a vizualizar e ter informações algum erro na hora de debugar.

**- Parâmetros:**
 - **erroTxt:** código de Erro que irá aparecer na tela do display;
      - 404: Problema ao conectar com o Host;
      - 401: Erro ao salvar o arquivo via php;
      - 425: Client Timeout ao enviar dados;
      - 666 Problema ao criar as tarefas do freeRTOS;
 - **TimeDelay:** Tempo de exibição da informação no display.

1.9) **configModeCallback(WiFiManager myWiFiManager)**

Função de Callback do WiFiManager para quando o AcessPoint é criado.

1.9.1) **saveConfigCallback()**

Função de Callback do WiFiManager para quando uma nova conexão WiFi for cadastrada.

### 2) **Funções com retorno float**
2.1) **float readPrototipo()**

Função que retorna o valor da leitura do sensor no pino digital configurado no microcontrolador.

2.2) **float readRefSensor()**

Função que retorna o valor de vazão captado pelo sensor de referência.

2.3) **float transUnit(float protoData)**

Função que transforma o valor em L/min.

**- Parâmetros:**
   - **protoData:** Valor que será transformado em L/min.

2.4) **float difValues(float valueA, float valueB)**

Função que calcula a diferença percentual de um Valor A em relação a um Valor B.

**- Parâmetros:**
  - **valueA:** Valor A que irá ser usado para tirar a referência percentual;
  - **valueB:** Valor B que irá ser usado como referência.

### 3) **Funções de chamada das ISR**
3.1) **void WiFiISRCallback()**

Procedimento ISR para recadastrar uma rede WiFi no Microcontrolador através do WiFiManager.

## **Contexto Global**

O programa é feito utilizando o FreeRTOS, que é um Sistema Operacional de tempo real para microcontroladores. O sistema é configurado em 4 tasks (tarefas) responsáveis pela leitura, tratamento, visualização e compartilhamento dos dados coletados. Esse esquema pode ser visualizado pela imagem abaixo.

![Slide3](https://user-images.githubusercontent.com/90044415/142294967-59a4ff23-ccfa-4670-ac6d-b69640ade7b2.PNG)

## **void Setup**

Na função void setup() é realizado a configuração de WiFi e pinos do Microcontroladorr.
![Slide2](https://user-images.githubusercontent.com/90044415/142295066-4816dbb1-7905-4775-8dd6-a81f7c84f9b3.PNG)

## Filas

- **xFilaDisplay:** Fila responsável por enviar informações da vTaskDataSensor para a vTaskDisplay
- **xFilaDisplay:** Fila responsável por enviar informações da vTaskDataSensor para a vTaskMySQL


## Tarefas (Tasks)
### **1- vTaskDataSensor**

Tarefa responsável pela leitura e tratamento dos dados do sensor e compartilhar essas informações com o display pela xFilaDisplay e com a comunicação MySQL pela xFilaMySQL.
![Slide4](https://user-images.githubusercontent.com/90044415/142295085-3e1a6c96-dedc-4292-93fd-25bbff802081.PNG)


### **2- vTaskDisplay**

Tarefa responsável por mostrar o dado na display do microcontrolador.

![Slide5](https://user-images.githubusercontent.com/90044415/142294893-1d72c636-75c2-4483-be8c-0d1cbcc66ce5.PNG)


### **3- vTaskMySQL**

Tarefa Responsável por enviar os dados do sensor ao banco de dados do Web APP.

![Slide6](https://user-images.githubusercontent.com/90044415/142294921-f46fd98a-892b-417f-a35b-f82835bed615.PNG)


## **4- vTaskWiFiReset**

Tarefa responsável por reconfigurar dados de WiFi no Microcontrolador.

![Slide7](https://user-images.githubusercontent.com/90044415/142294928-4a3c3c80-de07-423e-8824-73cfcbffee25.PNG)


## Esquema do servidor

O servidor utilizado é um computador conectado ao LocalHost, utilizando como base o XAMPP na porta 80. O servidor tem 4 arquivos .php responsáveis por conectar a um banco de dados e salvar dados de vazão ou algum novo medidor. O banco de dados é mantido pelo Heroku. O código pode ser visto pelo link do [WebServer mantido pelo XAMPP](https://github.com/saulooficinas/conexaoWebServer).

![Slide8](https://user-images.githubusercontent.com/90044415/142295265-4b7b1064-cf69-4c09-939d-928eee313613.PNG)



## Necessidades no momento

- [ ] Montagem física do sensor;
- [ ] Código para chamar o nome do ID acessado pelo WiFi Manager;
- [X] Adicionar rotina de configurar WiFi;
- [ ] Problemas com as leituras analógicas do sensor de tensão;
- [X] Programa para utilizar um medidor de vazão como referência e manipular as bobinas;
- [ ] Atualizar index.php;

## Circuito das Bobinas do Projeto.


## PERMISSÃO PARA UTILIZAR CÓDIGOS POR TERCEIROS

 OBS: *Ninguém além da equipe e das empresas parceiras tem o direito de utilizar este código sem permissão e para benefício próprio. Caso algo assim ocorra, será sujeito a um tratamendo jurídico.*
 
 - [ ] TCC voltado ao projeto...;
 
 ## EQUIPE:
 * Prof. Me. Mahelvson Bazilio Chaves(Orientador);
 * Samuel (cursando Eng. Elétrica);
 * Michele (Cursando Médio Técnico em Informática);
 * Matheus (Cursando Médio Técnico em Eletrotécnica);
 * Saulo (Cursando 2º período de  Eng. Elétrica e finalizando Técnico em Elétrotécnica);
 * Jéssica Barros (Cursando Eng. Cívil):
 * Emanuel (Cursando Médio Técnico em Eletrotécnica);

## Apoiadores.
