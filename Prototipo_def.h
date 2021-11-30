// Defines de variáveis.
#define CAMPO_BOBINA 0.006
#define DIAM_TUBE 0.06
#define MAT_PI 3.1415926536
#define CONST_SENSOR 1

//Valor de interações do filtro de média móvel
#define max_int 100

//Pino para configurar WiFi e interrupção do sensor de referência.
#define pin_ISR_WiFi 13

//define saídas
#define PIN_PROTOTIPO 34
#define IP_SENSOR 1


//Definido pinos do PWM
#define GPIO_PWM0A_OUT 12
#define GPIO_PWM0B_OUT 14

//Endereço host
const char* host = "192.168.0.109";

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
  uint8_t pino;

  //Valor encontrado neste pino
  double valor;
};

//Obs: 0 -> Ref
//     1 -> Prot

/*======================|| Protótipo das funções que irei utilizar||============================*/
//PROCEDIMENTOS
void initDisplay();
void initFreeRTOS();
void printDisplay(float protoData);
void initSaidas();
void initWiFi();
void initPWMbobinas();
void creditsProto();
void initBarDisplay(int iniLoad, int finLoad, char* infChar);
void errorMessage(int erroTxt, uint8_t TimeDelay);

//Funções de retorno float
float readPrototipo();
double transUnit(float protoData);
float difValues(float valueA, float valueB);


//Funções de controlo das bobinas.
static void bobina_ascendente(mcpwm_unit_t mcpwm_num, mcpwm_timer_t, float duty_cicle);
static void bobina_descendente(mcpwm_unit_t mcpwm_num, mcpwm_timer_t, float duty_cicle);
static void bobina_desligada(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num);


/*======================|| CÓDIGOS DE ERROS CRÍTICOS ||============================*/
/*
  404: Problema com o Host;
  401: Erro ao salvar arquivo;
  666: Problema ao criar tarefas do freeRTOS;
  425: ClientTimeout ao enviar dados.
  411: Erro no servidor
  
*/ 
