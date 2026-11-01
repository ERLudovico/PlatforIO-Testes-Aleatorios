// // This code should upload on Receiver and Temetria Nano(TLMT-A)

// Include Arduino library for basic functions and RX-TX-common.h for shared definitions
#include <Arduino.h>
#include <RX-TX-common.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
//Define all variables and constants before they are used

//const bool __DEBUG__ = true; // Habilita mensagens de depuracao no monitor serial.


// Declare all functions before they are used
void RadioRXInicialization() {
  if (__DEBUG__) {
    Serial.print(F("RX iniciando; CE="));
    Serial.print(CE_PIN);
    Serial.print(F(", CSN="));
    Serial.print(CSN_PIN);
    Serial.println(F(")"));
  }
  // Repete o diagnostico para nao deixar o monitor vazio se o radio falhar.
  while (!radio.begin()) {
    Serial.println(F("RX: radio nao encontrado. Confira modelo, CE/CSN e alimentacao."));
    delay(2000);
  }
  radio.openReadingPipe(0, address);
  radio.setPALevel(__RADIO_POWER__);
  radio.startListening();
  if (__DEBUG__) {
    Serial.println(F("RX pronto; aguardando Hello World."));
  }
}

//Setup function runs once when the microcontroller is powered on or reset
void setup() {
  Serial.begin(__SPEED_SERIAL__);
  delay(500);
  RadioRXInicialization();
}

void ReceiveMessage() {
  static uint32_t ultimoAviso = 0;
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    text[sizeof(text) - 1] = '\0'; // Garante o fim da string recebida.
    if (__DEBUG__) {
      Serial.print("Received: ");
      Serial.println(text);
    }
    ultimoAviso = millis();
  }
  // Mesmo sem pacotes, confirma que a placa e a serial estao funcionando.
  if (millis() - ultimoAviso >= 3000) {
    if (__DEBUG__) {
      Serial.println(F("RX: aguardando dados do TX..."));
    }
    ultimoAviso = millis();
  }
}

//Loop function runs repeatedly after setup() is complete
void loop() {
  ReceiveMessage();
}


