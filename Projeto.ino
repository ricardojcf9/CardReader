#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN  10

#define STATE_STARTUP       0
#define STATE_STARTING      1
#define STATE_WAITING       2
#define STATE_SCAN_INVALID  3
#define STATE_SCAN_VALID    4
#define STATE_SCAN_MASTER   5
#define STATE_ADDED_CARD    6
#define STATE_REMOVED_CARD  7
#define STATE_SCAN_AFONSO   8
#define STATE_SCAN_RICARDO  9

#define REDPIN 4
#define GREENPIN 5    
#define BLUEPIN 6
#define RELAY 3

const int cardArrSize = 10;
const int cardSize    = 4;
byte cardArr[cardArrSize][cardSize];
byte masterCard[cardSize] = {195,86,137,22};   //Change Master Card ID
byte AfonsoCard[cardSize] = {5,130,72,217}; //Afonso
byte RicardoCard[cardSize] = {5,141,116,205}; //Ricardo
byte readCard[cardSize];
byte cardsStored = 0; 

// Create MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);
// Set the LCD I2C address

byte currentState = STATE_STARTUP;
unsigned long LastStateChangeTime;
unsigned long StateWaitTime;

//------------------------------------------------------------------------------------
int readCardState()
{
  
  int index;

  for(index = 0; index < 4; index++) {
    readCard[index] = mfrc522.uid.uidByte[index];
  }

   Serial.print("Tag: ");
  for(index = 0; index < 4; index++) {
    readCard[index] = mfrc522.uid.uidByte[index];

    
    Serial.print(readCard[index]);
    if (index < 3) {
      Serial.print(",");
    }
  }
  Serial.println(" ");

 
  //Check Master Card
  if ((memcmp(readCard, masterCard, 4)) == 0) {
    return STATE_SCAN_MASTER;
  }

  // Desativar relay (abrir o cofre) quando o cartão Afonso é lido
  if ((memcmp(readCard, AfonsoCard, 4)) == 0) {
        digitalWrite(GREENPIN, HIGH);
        Serial.println("Authorized access");
        Serial.print("Message : ");
        Serial.println("Hello Afonso.");
        Serial.println();
        digitalWrite(RELAY, LOW);
        delay(3000);
        digitalWrite(GREENPIN, LOW);
        digitalWrite(RELAY,HIGH);
    return STATE_SCAN_AFONSO;
  }

  // Desativar relay (abrir o cofre) quando o cartão Ricardo é lido
  if ((memcmp(readCard, RicardoCard, 4)) == 0) {
        digitalWrite(GREENPIN, HIGH);
        Serial.println("Authorized access");
        Serial.print("Message : ");
        Serial.println("Hello Ricardo.");
        Serial.println();
        digitalWrite(RELAY, LOW);
        delay(3000);
        digitalWrite(GREENPIN, LOW);
        digitalWrite(RELAY,HIGH);
    return STATE_SCAN_RICARDO;
  }

  if (cardsStored == 0) {
    return STATE_SCAN_INVALID;
  }

  for(index = 0; index < cardsStored; index++) {
    if ((memcmp(readCard, cardArr[index], 4)) == 0) {
      return STATE_SCAN_VALID;
    }
  }

 return STATE_SCAN_INVALID;
}

//------------------------------------------------------------------------------------
// Permite o registo de um novo cartão após o cartão ADMIN ter sido lido

void addReadCard() {
  int cardIndex;
  int index;

  if (cardsStored <= 20) {
    cardsStored++;
    cardIndex = cardsStored;
    cardIndex--;
  }

  for(index = 0; index < 4; index++) {
    cardArr[cardIndex][index] = readCard[index];
  }
}

//------------------------------------------------------------------------------------
// Permite anular o registo de um cartão registado após o cartão ADMIN ter sido lido

void removeReadCard() {     
  int cardIndex;
  int index;
  boolean found = false;
  
  for(cardIndex = 0; cardIndex < cardsStored; cardIndex++) {
    if (found == true) {
      for(index = 0; index < 4; index++) {
        cardArr[cardIndex-1][index] = cardArr[cardIndex][index];
        cardArr[cardIndex][index] = 0;
      }
    }
    
    if ((memcmp(readCard, cardArr[cardIndex], 4)) == 0) {
      found = true;
    }
  }

  if (found == true) {
    cardsStored--;
  }
}

//------------------------------------------------------------------------------------
// Atualiza o estado atual

void updateState(byte aState) {
  if (aState == currentState) {
    return;
  }

  // Faz a mudança de estado
  switch (aState) {
    case STATE_STARTING:
      StateWaitTime = 1000;
      digitalWrite(REDPIN, HIGH);
      digitalWrite(GREENPIN, LOW);
      digitalWrite(BLUEPIN, LOW);
      digitalWrite(RELAY,HIGH);
      break;
      
    case STATE_WAITING:
      StateWaitTime = 0;
      digitalWrite(REDPIN, LOW);
      digitalWrite(GREENPIN, LOW);
      digitalWrite(BLUEPIN, LOW);
      digitalWrite(RELAY,HIGH);
      break;
    
    case STATE_SCAN_INVALID:
      if (currentState == STATE_SCAN_MASTER) {
        addReadCard();
        aState = STATE_ADDED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDPIN, LOW);
        digitalWrite(GREENPIN, LOW);
        
      } else if (currentState == STATE_REMOVED_CARD) {
        return;
        
      } else {
        Serial.println("Authorized denied\n");
        StateWaitTime = 2000;
        digitalWrite(REDPIN, HIGH);
        digitalWrite(GREENPIN, LOW);
      }
      break;
          
    case STATE_SCAN_VALID:
      if (currentState == STATE_SCAN_MASTER) {
        removeReadCard();
        aState = STATE_REMOVED_CARD;
        StateWaitTime = 2000;
        digitalWrite(REDPIN, LOW);
        digitalWrite(GREENPIN, LOW);
        
      } else if (currentState == STATE_ADDED_CARD) {
        return;
        
      } else if (currentState == STATE_SCAN_AFONSO) {
        return;
        
      } else if (currentState == STATE_SCAN_RICARDO) {
        return;
        
      } else {
        StateWaitTime = 2000;
        Serial.println("Authorized access");
        Serial.println("Hello new user!\n");
        digitalWrite(REDPIN, LOW);
        digitalWrite(GREENPIN, HIGH);
        digitalWrite(RELAY,LOW);
      }
      break;

    case STATE_SCAN_MASTER:
      StateWaitTime = 5000;
      digitalWrite(REDPIN, LOW);
      digitalWrite(GREENPIN, LOW);
      digitalWrite(BLUEPIN, HIGH);
      break;
  }

  currentState = aState;
  LastStateChangeTime = millis();
}

void setup() {
  SPI.begin();         // Inicializar SPI Bus
  mfrc522.PCD_Init();  // Inicializar o módulo MFRC522

  LastStateChangeTime = millis();
  updateState(STATE_STARTING);

  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(RELAY, OUTPUT);

  Serial.begin(9600);
}

void loop() {

  byte cardState;

  if ((currentState != STATE_WAITING) && (StateWaitTime > 0) && (LastStateChangeTime + StateWaitTime < millis())) {
    updateState(STATE_WAITING);
  }

  // Procurar por cartões no leitor
  if ( ! mfrc522.PICC_IsNewCardPresent()) { 
    return; 
  } 
  
  // Seleciona um cartão
  if ( ! mfrc522.PICC_ReadCardSerial()) { 
    return; 
  }

  cardState = readCardState();
  updateState(cardState);
}
