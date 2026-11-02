#include <RX-TX-common.h>

// Cria os objetos globais uma única vez por firmware.
// Radio, pinos e endereço devem ser os mesmos em todos os arquivos .cpp.
int __CE_PIN__ = 10; // Pino CE do nRF24L01+ (D10 no Arduino Nano integrado).
int __CSN_PIN__ = 9; // Pino CSN do nRF24L01+ (D9 no Arduino Nano integrado).
RF24 __RADIO__(__CE_PIN__, __CSN_PIN__ );  // CE = D10; CSN = D9.
const byte address[6] = "00001";
int __CANAL__ = 76; // Canal do rádio (0 a 127).
// A biblioteca ja define o enum rf24_datarate_e. Escolha apenas UMA taxa:
// RF24_1MBPS, RF24_2MBPS ou RF24_250KBPS; use a mesma nas duas placas.
const rf24_datarate_e __DATA_RATE__ = RF24_1MBPS;

//Serial speed for monitoring
const unsigned long __SPEED_SERIAL__ = 115200UL;
const unsigned int __RADIO_POWER__ = 3; // Potência do rádio (0 a 3, de baixa a alta).
const bool __DEBUG__ = true; // Habilita mensagens de depuração no monitor serial.
const uint32_t __INTERVALO_MS__ = 100; // Intervalo entre inicios de troca feitos por A: 1000 ms = 1 segundo.
const uint32_t __TIMEOUT_MS__ = 200; // Depois de enviar, A aguarda ate 200 ms pelos DADOS de resposta de B. Esse timeout evita esperar indefin

void configurarRadio() {
  
    Serial.begin(__SPEED_SERIAL__);

  if (!__RADIO__.begin()) {
    if (__DEBUG__) {
      Serial.println(F("Radio nao encontrado. Confira alimentacao e CE/CSN."));
    }
    while (true) {
      delay(1000);
    }
  }
  
  __RADIO__.setChannel(__CANAL__);
  __RADIO__.setDataRate(__DATA_RATE__);
  __RADIO__.setPALevel(__RADIO_POWER__);
  __RADIO__.setAutoAck(true);
  __RADIO__.setRetries(5, 15);
  __RADIO__.setPayloadSize(sizeof(PacoteCTRLA));
  __RADIO__.setPayloadSize(sizeof(PacoteTLMTA));
  pinMode(4, OUTPUT);
}