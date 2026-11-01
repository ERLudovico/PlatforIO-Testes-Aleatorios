// This code should upload on Remote Control (CTRL-A)

// Include Arduino library for basic functions and RX-TX-common.h for shared definitions
#include <Arduino.h>
#include <RX-TX-common.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <limits.h>

/*
//Define all variables and constants before they are used
*/
int count = 0; // Conta as tentativas de envio.
//const bool DEBUG = true; // Habilita mensagens de depuracao no monitor serial.
/*
// Declare all functions before they are used
*/
void RadioTXInicialization() {
    if (__DEBUG__) {
        Serial.print(F("TX iniciando; CE="));
        Serial.print(CE_PIN);
        Serial.print(F(", CSN="));
        Serial.print(CSN_PIN);
        Serial.println(F(")"));
    }
  // Testa o radio LOCAL antes de tentar falar com a outra placa.
  while (!radio.begin()) {
    Serial.println(F("TX: radio nao encontrado. Confira modelo, CE/CSN e alimentacao."));
    delay(2000);
  }
  radio.openWritingPipe(address);
  radio.setPALevel(__RADIO_POWER__);
  radio.stopListening();
  
  if (__DEBUG__) {
    Serial.println(F("TX pronto; Enviando Hello World"));
  }
}

void SendMessage() {
  // Reinicia em 1 ao atingir o limite do inteiro, evitando estouro.
  //count = (count == INT_MAX) ? 1 : count + 1;
  // Monta uma mensagem com texto e contador para o RX exibir.
  char text[32] = "";
  snprintf(text, sizeof(text), "Hello World (count=%d)", count);
  bool confirmado = radio.write(text, sizeof(text));
  
  if (__DEBUG__) {
    Serial.print(F("TX: "));
    Serial.print(text);
    Serial.println(confirmado ? F("; ACK recebido")
                              : F("; sem ACK do RX"));
  }
}

/*
//Setup function runs once when the microcontroller is powered on or reset
*/
void setup() {
  Serial.begin(__SPEED_SERIAL__);
  delay(500); // Da tempo para acompanhar a inicializacao no monitor.
  // Testa o radio LOCAL antes de tentar falar com a outra placa.
  RadioTXInicialization();
  
}

//Loop function runs repeatedly after setup() is complete
void loop() {
  // put your main code here, to run repeatedly:
   // Reinicia em 1 ao atingir o limite do inteiro, evitando estouro.
  count = (count == INT_MAX) ? 1 : count + 1;
  SendMessage();  
  delay(1000);
}
