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
- 

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

1.9) **configModeCallback(WiFiManager *myWiFiManager)**

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

## Definicão das tarefas utilizadas no projeto


## Esquema de processamento de dados.


## Necessidades no momento

- [ ] Montagem física dos sensores;
- [ ] Código para chamar o nome do ID acessado pelo WiFi Manager;
- [ ] Gerar Tarefa de tratamento de Interrupções para o sensor de vazão de referência;
- [x] Adicionar rotina de configurar WiFi;
- [ ] Adicionar Rotina de Auto-Testes do Medidor;


## PERMISSÃO PARA UTILIZAR CÓDIGOS POR TERCEIROS

 OBS: *Ninguém além da equipe e das empresas parceiras tem o direito de utilizar este código sem permissão e para benefício próprio. Caso algo assim ocorra, será sujeito a um tratamendo jurídico.*
 
 - [ ] TCC voltado ao projeto;
 
 ## EQUIPE:
 * Prof. Me. Mahelvson Bazilio Chaves(Orientador);
 * Samuel (cursando Eng. Elétrica);
 * Michele (Cursando Médio Técnico em Informática);
 * Matheus (Cursando Médio Técnico em Eletrotécnica);
 * Saulo (Cursando 2º período de  Eng. Elétrica e finalizando Técnico em Elétrotécnica);
 * Jéssica Barros (Cursando Eng. Cívil):
 * Emanuel (Cursando Médio Técnico em Eletrotécnica);
