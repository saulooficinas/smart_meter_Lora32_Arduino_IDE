// Defines de variáveis.
#define CAMPO_BOBINA 10
#define DIAM_TUBE 0.06
#define MAT_PI 3.1415926536
#define CONST_SENSOR 1

//Valor de interações do filtro de média móvel
#define max_int 100

//Pino para configurar WiFi e interrupção do sensor de referência.
#define pin_ISR_WiFi 36
#define pin_ISR_Ref 37

//define saídas
#define PIN_PROTOTIPO 32
#define PIN_REF 33
#define IP_SENSOR 1


//Endereço host
const char* host = "192.168.0.107";

//Chars improtantes
char CharS[] = "iniciando saidas...";
char CharI[] = "iniciando ISR...";
char CharW[] = "iniciando WiFi...";
char CharF[] = "iniciando FreeRTOS...";

//Chars para WiFiManager
char ssidWM[] = "SMWF_AP";
char senhaWM[] = "12345678";

//Criando estrutura para enviar situação do protótipo e o sensor de referência
struct sensorRef
{
  //Endereço dos pinos
  uint8_t pino[2];

  //Valor encontrado neste pino
  float valor[2];
};

//Obs: 0 -> Ref
//     1 -> Prot

/*======================|| Protótipo das funções que irei utilizar||============================*/
//PROCEDIMENTOS
void initDisplay();
void initFreeRTOS();
void printDisplay(float protoData, float refData);
void initSaidas();
void initWiFi();
void creditsProto();
void initBarDisplay(int iniLoad, int finLoad, char* infChar);
void errorMessage(uint8_t erroTxt, uint8_t TimeDelay);

//Funções de retorno float
float readPrototipo();
float readRefSensor();
float transUnit(float protoData);
float difValues(float valueA, float valueB);

//Funções de chamada das ISR
void WiFiISRCallback();
