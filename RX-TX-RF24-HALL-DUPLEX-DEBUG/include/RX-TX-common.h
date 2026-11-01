#pragma once  // Evita incluir este cabeçalho repetidamente.
#include <Arduino.h>
#include <RF24.h>

// Declara o objeto global. Ele será criado em um arquivo .cpp.
extern RF24 radio;
extern const byte address[6];
extern int CE_PIN;  // Pino CE do nRF24L01+ (D10 no Arduino Nano integrado).
extern int CSN_PIN; // Pino CSN do nRF24L01+ (D9 no Arduino Nano integrado).

extern const unsigned long __SPEED_SERIAL__;// Velocidade da porta serial para monitoramento.
extern const unsigned int __RADIO_POWER__; // Potência do rádio (0 a 3, de baixa a alta).
extern const bool __DEBUG__; // Habilita mensagens de depuração no monitor serial.

// Formato compartilhado dos dados enviados e recebidos.
// As duas placas precisam usar os mesmos campos, na mesma ordem.
struct Pacote {
  uint32_t sequencia; // Identifica a troca de mensagens.
  uint16_t leitura;   // Leitura analogica da placa que envia.
};

// Impede que o pacote ultrapasse o limite de 32 bytes do radio.
static_assert(sizeof(Pacote) <= 32, "Pacote excede o limite do radio");
