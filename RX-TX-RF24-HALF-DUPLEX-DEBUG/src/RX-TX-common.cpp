#include <RX-TX-common.h>

// Cria os objetos globais uma única vez por firmware.
// Radio, pinos e endereço devem ser os mesmos em todos os arquivos .cpp.
int CE_PIN = 10; // Pino CE do nRF24L01+ (D10 no Arduino Nano integrado).
int CSN_PIN = 9; // Pino CSN do nRF24L01+ (D9 no Arduino Nano integrado).
RF24 radio(CE_PIN, CSN_PIN);  // CE = D10; CSN = D9.
const byte address[6] = "00001";

//Serial speed for monitoring
const unsigned long __SPEED_SERIAL__ = 115200UL;
const unsigned int __RADIO_POWER__ = 3; // Potência do rádio (0 a 3, de baixa a alta).
const bool __DEBUG__ = true; // Habilita mensagens de depuração no monitor serial.