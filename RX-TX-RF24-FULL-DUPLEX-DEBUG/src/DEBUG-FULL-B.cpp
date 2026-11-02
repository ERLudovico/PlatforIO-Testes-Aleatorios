/*
 * TRANSCEIVER-B: PLACA B - RESPONDE AOS PEDIDOS DE A
 *
 * DIFERENCAS ENTRE OS ARQUIVOS A E B:
 *   - A unica diferenca de codigo e RADIO_NODE: 0 em A e 1 em B.
 *   - Os demais comandos sao iguais; essa constante seleciona o papel.
 *   - A: escuta NANOA, envia para NANOB, inicia pedidos e controla timeout.
 *   - B: escuta NANOB, envia para NANOA e responde aos pedidos recebidos.
 *   - Ambas recebem e transmitem. A diferenca e quem INICIA a troca.
 *   - Procure [A/B] neste arquivo para localizar os efeitos dessa escolha.
 *
 * O que este programa faz:
 *   1. A placa A mede sua entrada analogica A0 e envia o resultado para B.
 *   2. A placa B recebe esse resultado, mede seu proprio A0 e responde a A.
 *   3. A placa A recebe a resposta. A troca se repete a cada segundo.
 *
 * Por que as placas se revezam:
 *   O nRF24 funciona em HALF-DUPLEX: transmite OU recebe em cada instante.
 *   As duas placas sao transceptores, mas nao transmitem simultaneamente.
 *   Neste exemplo, A sempre inicia e B apenas responde. Essa organizacao
 *   evita que as duas iniciem uma transmissao enquanto a outra nao escuta.
 *
 * Como usar:
 *   - Este arquivo e exclusivo da placa B: ela responde aos pedidos de A.
 *   - PlatformIO: grave usando o ambiente DEBUG-FULL-B.
 *   - Na outra placa, grave DEBUG-FULL-A.cpp pelo ambiente DEBUG-FULL-A.
 *   - IDE Arduino: instale RF24, copie este codigo para um sketch .ino
 *     e adicione RX-TX-common.h e RX-TX-common.cpp na pasta do sketch.
 *     A identidade ja esta definida neste arquivo; nao e preciso altera-la.
 *   - Abra o monitor serial em 115200 baud.
 *   - Confira CE/CSN no esquema da sua versao do RF Nano.
 *
 * Exemplo de sinal em A0, para Nano classico de 5 V:
 *   Potenciometro com extremos em 5 V e GND e cursor no A0 da mesma placa.
 *   A leitura vai de 0 a 1023. A0 solto fica flutuante e pode variar sozinho.
 *   Essa variacao nao significa necessariamente falha de comunicacao.
 */

// ---------------------------------------------------------------------------
// 1. BIBLIOTECAS: recursos usados pelo programa
// ---------------------------------------------------------------------------
#include <Arduino.h> // Serial, millis(), delay(), analogRead() e tipos Arduino.
#include <SPI.h>     // Comunicacao SPI entre o microcontrolador e o radio local.
#include <RF24.h>    // Comandos para configurar, transmitir e receber no nRF24.
#include "RX-TX-common.h" // Formato do pacote e declaracoes compartilhadas.

// SPI conecta o processador ao seu proprio radio; a ligacao entre as duas
// placas acontece pelo ar. No RF Nano integrado, as ligacoes SPI sao da placa.

// ---------------------------------------------------------------------------
// 2. IDENTIDADE: escolher qual papel esta placa desempenha
// ---------------------------------------------------------------------------
// Este arquivo tem identidade fixa: placa B (responde aos pedidos de A).
// Nao depende de build_flags ou de editar um #define antes de gravar.
// A constante seleciona o comportamento desta placa nos testes do loop().
// [A/B] DIFERENCA DIRETA: neste arquivo vale 1 (placa B);
// no outro arquivo vale 0. E isso que seleciona os caminhos abaixo.
constexpr uint8_t RADIO_NODE = 1; // 0 = A; 1 = B.

// ---------------------------------------------------------------------------
// 3. RADIO, ENDERECOS E TEMPOS: definir com quem falar e quando
// ---------------------------------------------------------------------------
// CE controla a operacao do radio; CSN seleciona o radio no barramento SPI.
// Mesmos pinos usados pela placa A: CE = D10 e CSN = D9.
// Confira a ligacao interna da sua versao do RF Nano antes de gravar.
// O objeto radio e criado uma unica vez em RX-TX-common.cpp.

// Cada placa tem um endereco de recepcao diferente, com cinco bytes uteis.
// O sexto byte de cada texto e o terminador '\0' da string em C++.
// A escuta NANOA e escreve para NANOB; B faz o caminho inverso.
const uint8_t ENDERECOS[2][6] = {"NANOA", "NANOB"};

// "No" significa um participante da rede, e nao um pino do Arduino.
// Se NO_LOCAL = 0, NO_REMOTO = 1; se NO_LOCAL = 1, NO_REMOTO = 0.
// constexpr indica valores constantes conhecidos durante a compilacao.
// [A/B] Neste arquivo: NO_LOCAL = 1 e NO_REMOTO = 0.
// Esses indices invertem automaticamente os enderecos de envio e recepcao.
constexpr uint8_t NO_LOCAL = RADIO_NODE;
constexpr uint8_t NO_REMOTO = 1 - NO_LOCAL;

// Intervalo entre inicios de troca feitos por A: 1000 ms = 1 segundo.
// Depois de enviar, A aguarda ate 200 ms pelos DADOS de resposta de B.
// Esse timeout evita esperar indefinidamente caso B desligue ou saia do alcance.
//constexpr uint32_t INTERVALO_MS = 1000;
//constexpr uint32_t TIMEOUT_MS = 200;

// ---------------------------------------------------------------------------
// 4. PACOTE: formato dos dados que viajam entre as placas
// ---------------------------------------------------------------------------
// Pacote e a verificacao de tamanho estao em RX-TX-common.h.
// Assim, A e B usam os mesmos campos e a mesma ordem dos dados.

// ---------------------------------------------------------------------------
// 5. ESTADO: lembrar o andamento da troca entre execucoes do loop()
// ---------------------------------------------------------------------------
// Estas variaveis ficam fora das funcoes para manter seus valores.
uint32_t sequencia = 0;          // A incrementa a cada novo pedido.
uint32_t ultimoEnvio = 0;        // Instante do ultimo inicio de envio por A.
uint32_t inicioEspera = 0;       // Instante usado para contar o timeout.
bool aguardandoResposta = false; // Impede A de iniciar outra troca pendente.

// ---------------------------------------------------------------------------
// 6. TRANSMITIR: concentrar a mudanca RX -> TX -> RX em uma funcao
// ---------------------------------------------------------------------------
// const Pacote & recebe o pacote por referencia, sem copiar a estrutura,
// e impede que esta funcao modifique seus campos.
// O retorno bool informa se o radio recebeu a confirmacao automatica (ACK).
bool transmitir(const PacoteTLMTA &pacote) {
  // Sai do modo de escuta para permitir a transmissao.
  __RADIO__.stopListening();

  // &pacote aponta para os bytes; sizeof informa quantos bytes enviar.
  // write() e bloqueante: espera o envio/ACK ou o fim das tentativas.
  // Portanto, millis() no loop nao torna esta chamada nao bloqueante.
  bool confirmado = __RADIO__.write(&pacote, sizeof(pacote));

  // Volta imediatamente a escutar para poder receber a proxima mensagem.
  __RADIO__.startListening();

  // ACK confirma recepcao pelo RADIO remoto. Nao e a resposta com a leitura
  // de A0 e nao prova que o programa remoto ja processou o pacote.
  return confirmado;
}



// ---------------------------------------------------------------------------
// 7. SETUP: inicializacao, executada uma vez ao ligar ou reiniciar
// ---------------------------------------------------------------------------
void setup() {
  // As mensagens ajudam a acompanhar envios, respostas e falhas.
  // F("texto") guarda textos constantes na memoria de programa, poupando RAM.
  /*
  Serial.begin(115200);

  // Inicializa SPI/radio e verifica a comunicacao com o chip LOCAL.
  // Sucesso aqui nao garante que a outra placa esteja ligada ou acessivel.
  if (!__RADIO__.begin()) {
    Serial.println(F("Radio nao encontrado. Confira alimentacao e pinos CE/CSN."));

    // Interrompe a operacao normal: nao adianta enviar sem radio local.
    // Depois de corrigir o problema, reinicie a placa para tentar novamente.
    while (true) { delay(1000); }
  }

  // As duas placas precisam usar o mesmo canal e a mesma taxa de dados.
  __RADIO__.setChannel(76);
  __RADIO__.setDataRate(__DATA_RATE__);

  // Potencia baixa e suficiente para comecar os testes com placas proximas.
  // A potencia nao precisa ser igual nas duas; nao define o endereco remoto.
  __RADIO__.setPALevel(RF24_PA_HIGH);

  // Habilita a confirmacao automatica do radio receptor. O hardware envia
  // esse ACK; ele e separado do pacote de resposta criado no loop().
  __RADIO__.setAutoAck(true);

  // Primeiro argumento: atraso entre repeticoes, em passos de 250 us.
  // Valor 5 corresponde a (5 + 1) * 250 = 1500 us.
  // Segundo argumento: ate 15 retransmissoes se o ACK nao chegar.
  __RADIO__.setRetries(5, 15);

  // Usa um payload de tamanho fixo, igual ao tamanho de Pacote nas duas placas.
  __RADIO__.setPayloadSize(sizeof(Pacote));
*/
  configurarRadio();
  // Destino das transmissoes: endereco da OUTRA placa.
  // [A/B] Placa B: destino das mensagens = NANOA.
  __RADIO__.openWritingPipe(ENDERECOS[NO_REMOTO]);

  // Pipe e um canal logico de recepcao dentro do mesmo radio/canal de RF.
  // Usamos pipe 1 para nosso endereco; pipe 0 e usado pela biblioteca no ACK
  // das transmissoes. Pipe 1 nao significa outro modulo ou outra frequencia.
  // [A/B] Placa B: endereco em que recebe mensagens = NANOB.
  __RADIO__.openReadingPipe(1, ENDERECOS[NO_LOCAL]);

  // Ambas comecam escutando. A saira desse modo quando iniciar o envio.
  __RADIO__.startListening();

  // Operador condicional: imprime A se NO_LOCAL == 0; caso contrario, B.
  Serial.print(F("Transceptor pronto: placa "));
  // [A/B] Aqui o monitor serial identifica esta placa como B.
  Serial.println(NO_LOCAL == 0 ? F("A") : F("B"));
}

PacoteTLMTA coletaDados(uint32_t sequencia) {
  uint16_t campoA = static_cast<uint16_t>(analogRead(A0));
  uint16_t campoB = static_cast<uint16_t>(analogRead(A1));
  PacoteTLMTA pacote = {sequencia, campoA, campoB};
  return pacote;
}  

void piscaLED(bool estado) {
  if (estado) {
    digitalWrite(4, HIGH); 
  } else {
    digitalWrite(4, LOW);
  }
  }
// ---------------------------------------------------------------------------
// 8. LOOP: recepcao, resposta e inicio de novas trocas
// ---------------------------------------------------------------------------
// loop() se repete continuamente. Primeiro atendemos os pacotes recebidos;
// depois, somente em A, verificamos timeout e necessidade de novo envio.
void loop() {
  // 8.1 RECEBER: available() indica dados na fila de recepcao do __RADIO__.
  // while, em vez de if, permite consumir os pacotes disponiveis na fila.
  while (__RADIO__.available()) {
    PacoteTLMTA recebido;

    // Copia os bytes recebidos para uma estrutura com o formato combinado.
    __RADIO__.read(&recebido, sizeof(recebido));
    if (__DEBUG__) {
      // Imprime os campos do pacote recebido, para acompanhar a troca de dados.
      Serial.print(F("RX sequencia="));
      Serial.print(recebido.sequencia);
      Serial.print(F(" Acelerador remoto="));
      Serial.print(recebido.campoA);
      Serial.print(F(" Leme remoto="));
      Serial.println(recebido.campoB);
    }

    // 8.2 EM B: todo pedido recebido gera uma resposta com o mesmo numero.
    // [A/B] ATIVO EM B: esta condicao e verdadeira; B mede A0 e responde.
    if (NO_LOCAL == 1) {
      // B pode receber antes de A terminar write() e voltar a escutar.
      // Esta pequena margem de 2 ms facilita a transicao neste exemplo.
      // delay() pausa o programa aqui; nao e uma espera de um segundo no loop.
      delay(2);

      // A sequencia vem de A; a leitura e feita no A0 de B neste instante.
      // static_cast converte explicitamente o resultado para uint16_t.
      //PacoteTLMTA resposta = {recebido.sequencia, static_cast<uint16_t>(analogRead(A0))};
      PacoteTLMTA resposta = coletaDados(++sequencia);
      bool confirmado = transmitir(resposta);
      (!confirmado)? piscaLED(true): piscaLED(false);
      
      if (__DEBUG__) {
        Serial.println(confirmado ? F("TX resposta: ACK recebido")
                                   : F("TX resposta: sem ACK"));
      }
    // 8.3 EM A: so encerra a espera se a resposta pertencer ao pedido atual.
    // Uma resposta atrasada de uma troca anterior pode aparecer no monitor,
    // mas nao deve ser confundida com a resposta que estamos aguardando.
    // [A/B] INATIVO EM B: B ja executou o primeiro ramo do if acima.
    }
    /*} else if (aguardandoResposta && recebido.sequencia == sequencia) {
      aguardandoResposta = false;
      Serial.println(F("Troca de dados concluida."));
    }
    */
  }

  // B nao inicia trocas por conta propria: apenas escuta e responde acima.
  // [A/B] INATIVO EM B: nao inicia pedidos nem executa este controle de tempo.
  /*
  if (NO_LOCAL == 0) {
    // millis() informa os milissegundos desde a inicializacao da placa.
    // Guardamos o instante para verificar os prazos sem delay(1000).
    uint32_t agora = millis();

    // 8.4 TIMEOUT: libera a espera quando a resposta demora demais.
    // A subtracao entre inteiros sem sinal permite contar o tempo mesmo
    // quando millis() volta a zero, aproximadamente a cada 49,7 dias,
    // desde que o programa continue verificando estes intervalos curtos.
    if (aguardandoResposta && agora - inicioEspera >= __TIMEOUT_MS__) {
      aguardandoResposta = false;
      Serial.println(F("Tempo esgotado aguardando dados da placa B."));
    }

    // 8.5 NOVO PEDIDO: exige as duas condicoes ao mesmo tempo (operador &&):
    //   - nao existir uma resposta pendente;
    //   - ter passado pelo menos um segundo desde o ultimo inicio de envio.
    // O intervalo e entre envios, nao um segundo depois de receber a resposta.
    if (!aguardandoResposta && agora - ultimoEnvio >= __INTERVALO_MS__) {
      ultimoEnvio = agora;

      // ++ incrementa ANTES de usar: o primeiro pedido recebe sequencia 1.
      // analogRead(A0) mede a entrada de A; troque esse campo se quiser enviar
      // outro dado, mantendo o formato correspondente na placa B.
      PacoteTLMTA envio = {++sequencia, static_cast<uint16_t>(analogRead(A0))};
      bool confirmado = transmitir(envio);
      Serial.print(F("TX sequencia="));
      Serial.println(sequencia);
      Serial.println(confirmado ? F("ACK recebido; aguardando dados de B.")
                                : F("Sem ACK; aguardando possivel resposta de B."));
      // Ausencia de ACK nao prova que B nao recebeu: o ACK pode se perder.
      // Por isso, aguardamos a resposta com dados mesmo quando write() falha.
      // A contagem dos 200 ms comeca depois do envio e das mensagens seriais.
      inicioEspera = millis();
      aguardandoResposta = true;
    }
  }
  */
}
