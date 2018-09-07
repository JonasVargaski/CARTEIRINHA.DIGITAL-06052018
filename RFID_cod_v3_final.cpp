/*
 * LCD RS pin to digital pin  7
 * LCD Enable pin to digital pin  6
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

Buzer pin 8
lcd led pin AN3

 * ------------------------------------
 *             MFRC522      Arduino      
 *             Reader/PCD   Uno/101      
 * Signal      Pin          Pin          
 * ------------------------------------
 * RST/Reset   RST          9            
 * SPI SS      SDA(SS)      10           
 * SPI MOSI    MOSI         11 / ICSP-4   
 * SPI MISO    MISO         12 / ICSP-1  
 * SPI SCK     SCK          13 / ICSP-3   
 */


#include <SPI.h>        // Inclui biblioteca para uso da conexao SPI FullDuplex -- protocolo RFID
#include <MFRC522.h>      // Inclui biblioteca para uso do modulo rfid
#include <LiquidCrystal.h>    // inclui biblioteca para uso do display LCD
#include <EEPROM.h>      // Inclui biblioteca para uso da EEPROM do ATMEGA328P
 
#define SS_PIN 10       // define pino de SlaveSelect (modulo rfid)
#define RST_PIN 9       // define pino de reset do modulo rfid
#define BUZZER_PIN 8      // define pino para Buzzer
#define BT_RESET A5       //Botao de reset.

#define ON 1          // mascara para HIGH
#define OFF 0         // mascara para LOW


#define QTD_USUARIOS 21        // SETAR QTD EXATA PARA FUNCIONAR CORRETAMENTE, todos os laços de repetiçao utilizam para iterar nos arrays

MFRC522 mfrc522(SS_PIN, RST_PIN);     // Create MFRC522 instance.
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);  // Create LCD instance.


const PROGMEM String UID[QTD_USUARIOS] = {            //Setar codigo UID dos TAG's .
                                       "A4 80 16 89"
                                      ,"FD 59 0B 89"
                                      ,"99 0C C5 49"};


const PROGMEM String NAME[QTD_USUARIOS] = {           //Setar Nome dos usuarios aqui, seguir na mesma sequencia de codigos.
                                       "JONAS VARGASKI"
                                      ,"DANIEL ALBERTON"
                                      ,"MARCELO BAINCO"};


boolean STATUS[QTD_USUARIOS] ;              //Array de flags para saber se usuario saiu ou entrou

///////////// VARIAVEIS GLOBAIS //////////////////
 
byte flag_posi =0 , qtd_alunos =0 ,posi =0, flag_t = 0, dia = 0, _dia=0;;                                                                          
unsigned long currentMillis, startMillis;

/////////// PROTÓTIPO DE FUNÇOES ///////////////


void qtd_alunos_faltantes();


 //////////////////////////////////////////////

void setup() 
{

  lcd.begin(20, 4);  // inicia lcd
  Serial.begin(9600);   // Inicia a serial
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
  pinMode(BUZZER_PIN,OUTPUT); // Configura pino do arduino como saida para utilizar o buzzer 

  for(byte i=0 ; i<QTD_USUARIOS ; i++){   //Restaura flags ao ligar o dispositivo.
    STATUS[i] = EEPROM.read(i);
  }

 }
               

void loop() 
{

////////////////   TIMER  //////////////////

  currentMillis = millis();          // Recebe tempo do timer interno do arduino
  if (currentMillis - startMillis >= 2500) { // periodo que o nome aparece na tela;
  startMillis = currentMillis;         // começo recebe atual para começar a contar novamente.

flag_t =1;                  // flag de estouro do timer.
flag_posi = QTD_USUARIOS*2;         // flag de posiçoes para iterar o array, força a percorrer todo os nomes.


while (flag_t && qtd_alunos != 0 && flag_posi){
   posi ++;
   flag_posi--;
   if(posi >= QTD_USUARIOS) posi =0;
   if(STATUS[posi]){
    lcd.clear();
    lcd.setCursor(0,3);
    lcd.print(NAME[posi]);
    flag_t = 0;
    return;
    }
  }
}

qtd_alunos_faltantes(); // Chama função para atualizar Quantidade de chekins restantes.

  lcd.setCursor(1,0);
  lcd.print(F("Aproxime o CARTAO!"));
  lcd.setCursor(0,2);
  lcd.print(F("Chekin restantes: "));
  lcd.print(qtd_alunos);

  delay(70);        // atraso para nao causar efeito no lcd.

/////////////////////////////////////////////////////////////////////////////////////
    if (analogRead(BT_RESET) < 100) { // Verifica a condição do botao, se pressionar zera tudo;
    for(byte i=0 ; i<QTD_USUARIOS ; i++){   //Restaura flags se alguem nao bater cartao ou nao retornar.
     EEPROM.write(i,0);                      // Grava flag
     EEPROM.write(254,_dia);           // grava dia na eeprom
     STATUS[i] = 0;                          // zera flags
    }
   digitalWrite(BUZZER_PIN,ON);                           // da sinal de "ERRO" no buzzer 
   lcd.clear();
   lcd.print(F("Reset OK !"));
   delay(1000);
   digitalWrite(BUZZER_PIN,OFF); 
   lcd.clear();
    return;
  }

    
  // Procura por cartao RFID
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Seleciona o cartao RFID
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  
   String conteudo= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));   //Corrige o codigo para tornar legivel
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();                           // passa para letras maiusculas para nao haver problemas de comparaçao

  ///////////////////////////////////////////////////////////////////////////////// 

for(int i=0; i<QTD_USUARIOS; i++){                        // percorre todo o array comparando os UIDS
  if (conteudo.substring(1) == UID[i]) //UID do Cartao
  {
    lcd.clear();
    STATUS[i] = !STATUS[i];                        // Inverte a flag de status
    
    if(STATUS[i]){                                // se for 1 entao aluno saiu
      EEPROM.write(i,STATUS[i]);          // salva flag na memoria nao volatil
      lcd.setCursor(0,0);
      lcd.print(NAME[i]);
      lcd.setCursor(7,2);
      lcd.print(F("SAIDA"));
      digitalWrite(BUZZER_PIN,ON);         // da sinal de "Ok" no buzzer
      delay(140);
      digitalWrite(BUZZER_PIN,OFF);
      delay(1200);
      lcd.clear();
      return;
    }
      EEPROM.write(i,STATUS[i]);
      lcd.setCursor(0,0);                           //Se for 0 entao aluno retornou
      lcd.print(NAME[i]);             // salva flag na memoria nao volatil
      lcd.setCursor(6,2);
      lcd.print(F("RETORNO"));
      digitalWrite(BUZZER_PIN,ON);
      delay(140);
      digitalWrite(BUZZER_PIN,OFF);      // da sinal de "Ok" no buzzer
      delay(1200);
      lcd.clear();
      return;
    }
  }
   lcd.clear();                                 // Se nao existir entao avisa e exibe codigo da tag
   lcd.setCursor(0,0);
   lcd.print(F(" TAG NAO CADASTRADA "));
   lcd.setCursor(3,2);
   lcd.print(conteudo);
   digitalWrite(BUZZER_PIN,ON);                           // da sinal de "ERRO" no buzzer
      delay(250);
      digitalWrite(BUZZER_PIN,OFF);
      delay(100);
      digitalWrite(BUZZER_PIN,ON);
      delay(250);
      digitalWrite(BUZZER_PIN,OFF);
      delay(100);
      digitalWrite(BUZZER_PIN,ON);
      delay(250);
      digitalWrite(BUZZER_PIN,OFF);
   delay(2500);
   lcd.clear();
}


////////////////////////////////////////////////////////////////////////////////////////


void qtd_alunos_faltantes(){ 
   qtd_alunos = 0;                              //Calcula quantos alunos tem pra voltar, usando a flag STATUS.
  for(int i =0; i<QTD_USUARIOS ; i++){
   if(STATUS[i] != 0){
    qtd_alunos++;                               // atribui valor a uma variavel global, usada em outro processos
    }
  }
}  

//////////////////////////////////////////////////////////////////////////////////////
 
   
   