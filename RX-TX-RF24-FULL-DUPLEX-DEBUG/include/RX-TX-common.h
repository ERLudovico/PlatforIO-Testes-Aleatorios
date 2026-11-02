#pragma once  // Evita incluir este cabeçalho repetidamente.
#include <Arduino.h>
#include <RF24.h>

// Declara o objeto global. Ele será criado em um arquivo .cpp.
extern RF24 __RADIO__;
extern const byte address[6];
extern int __CE_PIN__;  // Pino CE do nRF24L01+ (D10 no Arduino Nano integrado).
extern int __CSN_PIN__; // Pino CSN do nRF24L01+ (D9 no Arduino Nano integrado).
extern int __CANAL__;    // Canal do rádio (0 a 127).
extern const rf24_datarate_e __DATA_RATE__; // Tipo de taxa esperado por setDataRate().

extern const unsigned long __SPEED_SERIAL__;// Velocidade da porta serial para monitoramento.
extern const unsigned int __RADIO_POWER__; // Potência do rádio (0 a 3, de baixa a alta).
extern const bool __DEBUG__; // Habilita mensagens de depuração no monitor serial.
extern const uint32_t __INTERVALO_MS__;
extern const uint32_t __TIMEOUT_MS__;

// Formato compartilhado dos dados enviados e recebidos.
// As duas placas precisam usar os mesmos campos, na mesma ordem.
/*
struct Pacote {
  uint32_t sequencia; // Identifica a troca de mensagens.
  uint16_t leitura;   // Leitura analogica da placa que envia.
};
*/
struct PacoteCTRLA {
  uint32_t sequencia; // Identifica a troca de mensagens.
  uint16_t campoA;   // Leitura analogica da placa que envia.
  uint16_t campoB;
  
};

struct PacoteTLMTA {
  uint32_t sequencia; // Identifica a troca de mensagens.
  uint16_t campoA;   // Leitura analogica da placa que envia.
  uint16_t campoB;
};


// Impede que o pacote ultrapasse o limite de 32 bytes do radio.
static_assert(sizeof(PacoteCTRLA) <= 32, "PacoteCTRLA excede o limite do radio");
static_assert(sizeof(PacoteTLMTA) <= 32, "PacoteTLMTA excede o limite do radio");

// Declara Funcoes Globais. Elas serão definidas em um arquivo .cpp.
void configurarRadio();
