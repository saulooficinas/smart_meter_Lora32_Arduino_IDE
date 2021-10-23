// Defines de variáveis.
#define CAMPO_BOBINA 10
#define DIAM_TUBE 0.06
#define MAT_PI 3.1415926536
#define CONST_SENSOR 1

//Valor de interações do filtro de média móvel
#define max_int 100

//define saídas
#define PIN_PROTOTIPO 32
#define PIN_REF 33
#define IP_SENSOR 1

//Defines do OLED
#define OLED_ADDR 0x3c

//Define das linhas
#define OLED_LINE1 0
#define OLED_LINE2 10
#define OLED_LINE3 208
#define OLED_LINE4 30
#define OLED_LINE5 40

//define tamanhos
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//Endereço host 
const char* host="192.168.0.108";

//Chars improtantes
char CharS[] = "iniciando saidas...";
char CharI[] = "iniciando ISR...";
char CharW[] = "iniciando WiFi...";
char CharF[] = "iniciando FreeRTOS...";

//Chars para WiFiManager
char ssidWM[]="SWMF_AP";
char senhaWM[]="12345678";

//Criando estrutura para enviar situação do protótipo e o sensor de referência
struct sensorRef
{
  //Endereço dos pinos
  int pino[2];

  //Valor encontrado neste pino
  float valor[2];
};

//Obs: 0 -> Ref
//     1 -> Prot

/*======================|| Protótipo das funções que irei utilizar||============================*/
//PROCEDIMENTOS
void initDisplay();
void initFreeRTOS();
void printDisplay(float a, float b);
void initSaidas();
void initWiFi();
void creditsProto();
void barDisplay(int a, int b, char* v);

//Funções de retorno float
float readPrototipo();
float readRefSensor();
float transUnit(float d);
float difValues(float a, float b);
