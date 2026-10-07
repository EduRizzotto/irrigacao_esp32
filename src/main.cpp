#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include "time.h"


WiFiServer server(80);

#define SS_PIN 5  
#define RST_PIN 22 

// calcular o tempo
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

// Agendamento de horario
int horaAgendada = -1;
int minutoAgendado = -1;


unsigned long ultimoTempoRelogio = 0;
bool jaAcionouAgendado = false;

int ledVerde = 12;
int ledVermelho = 13;
int botao = 14;
int pinRele = 26;

int verde_vermelho = 1;
int ultimo_verde_vermelho = 1;

unsigned long tempoInicio = 0; 
unsigned long tempoMax = 15000;
bool sistemaLiberado;
String statusSite = "Aguardando cartao...";

const int pinoBuzzer = 27;

MFRC522 rfid(SS_PIN, RST_PIN); 

String uidDesejado = "7A A1 65 06"; 

void bipCartao() {
  tone(pinoBuzzer, 2500, 80); 
  delay(120);                 
  tone(pinoBuzzer, 2500, 80);  
  delay(80);
}

void bipLigar() {
  tone(pinoBuzzer, 1500, 100); 
  delay(130);
  tone(pinoBuzzer, 2000, 100); 
  delay(130);
  tone(pinoBuzzer, 2500, 150); 
  delay(150);
}

void bipDesligar() {
  tone(pinoBuzzer, 2500, 100); 
  delay(130);
  tone(pinoBuzzer, 2000, 100); 
  delay(130);
  tone(pinoBuzzer, 1500, 150); 
  delay(150);
}

void bipDesligarTotal() {
  tone(pinoBuzzer, 1200, 250);
  delay(300);
  tone(pinoBuzzer, 800, 400); 
  delay(400);
}

bool verificar_rfid() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    return false;
  }

  String uidLido = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidLido += (rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uidLido += String(rfid.uid.uidByte[i], HEX);
  }
  
  uidLido.trim();
  uidLido.toUpperCase();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1(); 

  Serial.print("Cartao detectado: ");
  Serial.println(uidLido);

  if (uidLido == uidDesejado) {
    return true;
  } else {
    return false;
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Falha ao obter horário");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  SPI.begin();       
  rfid.PCD_Init();   
  
  pinMode(ledVermelho, OUTPUT); 
  pinMode(ledVerde, OUTPUT);
  pinMode(botao, INPUT_PULLUP);
  pinMode(pinRele, OUTPUT);
  
  WiFi.begin("WiFi-Nome", "WiFi-Senha");
  
  Serial.print("Conectando");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.print("\nConectado");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
  Serial.print("\nServidor Conectado\n");

  // Configura o fuso horário e sincroniza a hora via internet (NTP)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}

void loop() {

  WiFiClient client = server.available(); 
  if (client) {
    String linhaAtual = "";
    String requisicaoCompleta = "";
    unsigned long tempoLimiteWeb = millis(); 
    
    while (client.connected() && (millis() - tempoLimiteWeb < 250)) {
      if (client.available()) {
        char c = client.read();
        requisicaoCompleta += c;
        
        if (c == '\n') {
          if (linhaAtual.length() == 0) {
            
            if (requisicaoCompleta.indexOf("GET /dados") != -1) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();
              
              client.print(statusSite);
              client.print("|");
              if (verde_vermelho == -1) {
                client.print("<span style='color:green; font-weight:bold;'>LED VERDE ATIVO</span>");
              } else {
                client.print("<span style='color:red; font-weight:bold;'>LED VERMELHO ATIVO</span>");
              }
            }

            else if (requisicaoCompleta.indexOf("GET /abrir") != -1) {
              if (!sistemaLiberado){
                sistemaLiberado = true;
                tempoInicio = millis();
                statusSite = "Sistema liberado";
                Serial.println("SISTEMA LIBERADO POR 10 SEGUNDOS!");
              }
              verde_vermelho *= -1;  
              delay(300);
              
              client.println("HTTP/1.1 303 See Other");
              client.println("Location: /");
              client.println("Connection: close");
              client.println();

            }
            else if (requisicaoCompleta.indexOf("GET /tempo?valor=") != -1){
              int inicioNumero = requisicaoCompleta.indexOf("valor=") + 6;
              int fimNumero = requisicaoCompleta.indexOf(" HTTP");

              String numeroTexto = requisicaoCompleta.substring(inicioNumero, fimNumero);
              int segundosEscolhidos = numeroTexto.toInt();

              tempoMax = segundosEscolhidos * 1000;

              Serial.print("Novo tempo alterado para: ");
              Serial.print(segundosEscolhidos);
              Serial.println(" segundos."); 

              client.println("HTTP/1.1 200 OK");
              client.println("Connection: close");
              client.println();
            }
            else if (requisicaoCompleta.indexOf("GET /agendar?h=") != -1) {
              // 1. Encontra a posição do "h=" e do "m=" na URL
              int posH = requisicaoCompleta.indexOf("h=") + 2;
              int posEComercial = requisicaoCompleta.indexOf("&m=");
              int posM = requisicaoCompleta.indexOf("m=") + 2;
              int posEspaco = requisicaoCompleta.indexOf(" HTTP");
              
              // 2. Recorta os números como texto
              String horaTexto = requisicaoCompleta.substring(posH, posEComercial);
              String minutoTexto = requisicaoCompleta.substring(posM, posEspaco);
              
              // 3. Converte o texto para número e salva nas variáveis globais
              horaAgendada = horaTexto.toInt();
              minutoAgendado = minutoTexto.toInt();
              
              Serial.print("Novo horário agendado para: ");
              Serial.print(horaAgendada);
              Serial.print(":");
              Serial.println(minutoAgendado);
              
              // 4. Dá o "OK" para o navegador parar de carregar
              client.println("HTTP/1.1 200 OK");
              client.println("Connection: close");
              client.println();
            }
            else {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();
              
              client.println(R"rawhtml(
                                <!DOCTYPE html>
                <html lang="pt-BR">

                <head>
                  <meta charset="UTF-8">
                  <!-- Tag essencial para telas de celular adaptarem o zoom e largura -->
                  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
                  <title>ESP32 Controle</title>
                  <style>
                    body {
                      font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
                      text-align: center;
                      margin: 0;
                      padding: 20px;
                      background-color: #121212;
                      color: #e0e0e0;
                      display: flex;
                      flex-direction: column;
                      align-items: center;
                      justify-content: center;
                      min-height: 100vh;
                      box-sizing: border-box;
                    }

                    .card {
                      background: #1e1e1e;
                      padding: 40px 30px;
                      border-radius: 16px;
                      border: 1px solid #333;
                      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
                      width: 100%;
                      max-width: 380px;
                      box-sizing: border-box;
                    }

                    h1 {
                      font-size: 1.8em;
                      margin-top: 0;
                      margin-bottom: 25px;
                      color: #ffffff;
                      font-weight: 600;
                      letter-spacing: 0.5px;
                    }

                    p {
                      font-size: 1.1em;
                      margin: 18px 0;
                      line-height: 1.6;
                      color: #a0a0a0;
                    }

                    strong {
                      color: #e0e0e0;
                      font-size: 0.9em;
                      text-transform: uppercase;
                      letter-spacing: 1px;
                    }

                    #status,
                    #estadoLed {
                      display: inline-block;
                      margin-top: 8px;
                      font-size: 1.2em;
                      color: #ffffff;
                    }

                    .btn {
                      padding: 16px 20px;
                      font-size: 1.1em;
                      font-weight: bold;
                      background-color: #00e676;
                      color: #121212;
                      border: none;
                      border-radius: 12px;
                      cursor: pointer;
                      width: 100%;
                      box-shadow: 0 4px 15px rgba(0, 230, 118, 0.2);
                      transition: all 0.2s ease;
                      margin-top: 30px;
                      text-transform: uppercase;
                      letter-spacing: 1px;
                    }

                    .btn:active {
                      background-color: #00c853;
                      transform: translateY(2px);
                      box-shadow: 0 2px 4px rgba(0, 200, 83, 0.2);
                    }

                    /* Estilizando o status que vem do backend pra combinar melhor com o tema escuro */
                    span[style*="color:red"] {
                      color: #ff5252 !important;
                      text-shadow: 0 0 8px rgba(255, 82, 82, 0.3);
                    }

                    span[style*="color:green"] {
                      color: #69f0ae !important;
                      text-shadow: 0 0 8px rgba(105, 240, 174, 0.3);
                    }
                  </style>
                  <script>
                    function atualizarDados() {
                      fetch('/dados')
                        .then(response => response.text())
                        .then(texto => {
                          let partes = texto.split('|');
                          document.getElementById('status').innerHTML = partes[0];
                          document.getElementById('estadoLed').innerHTML = partes[1];
                        });
                    }
                    setInterval(atualizarDados, 1000); // Atualiza a cada 1 segundo automaticamente
                  </script>
                </head>

                <body>
                  <div class="card">
                    <h1>Controle de Acesso</h1>
                    <p><strong>Status Atual:</strong><br><span id='status'>Carregando...</span></p>
                    <p><strong>Estado:</strong><br><span id='estadoLed'>Carregando...</span></p>
                    <a href='/abrir' style='text-decoration: none;'>
                      <button class="btn">Liberar Remotamente</button>
                    </a>

                    <div style="margin-top: 30px; padding-top: 20px; border-top: 1px solid #333;">
                      <p><strong>Tempo p/ Desligar automaticamente :</strong></p>
                      <label for="tempo-slider">Escolha o tempo:<br></label>
                      <!-- O input chama a função atualizarValor() cada vez que muda (oninput) -->
                      <input type="range" id="tempo-slider" name="tempo-slider" min="10" max="30" value="15" oninput="atualizarValor()">

                      <!-- Onde o número vai aparecer (começa em 20 que é o valor padrão) -->
                      <br>
                      <span id="valor-selecionado">15</span> segundos
                      <br>

                      <button class="btn" style="background-color:  #2196f3; margin-top: 15px;"
                        onclick="enviarTempo()">Selecionar</button>
                    </div>
                    <script>
                      function atualizarValor() {
                        const slider = document.getElementById('tempo-slider');
                        const textoValor = document.getElementById('valor-selecionado');
                        textoValor.innerText = slider.value;
                      }
                      function enviarTempo() {
                        const tempo = document.getElementById('tempo-slider').value;
                        fetch('/tempo?valor=' + tempo)
                          .then(response => {
                            alert("Tempo alterado para " + tempo + " segundos!");
                          });
                      }
                    </script>
                    <style>
                      /* Container apenas para organizar e centralizar */
                      .campo-hora {
                        display: flex;
                        flex-direction: column;
                        align-items: center;
                        /* Centraliza o texto e a caixa de hora */
                        font-family: 'Segoe UI', sans-serif;
                        gap: 12px;
                        width: 100%;
                        margin-top: 30px;
                        padding-top: 20px;
                        border-top: 1px solid #333;
                        /* Cria aquela linha de separação */
                      }

                      /* Estilo para os inputs de número seguros (Horas e Minutos) */
                      .input-numero {
                        background-color: #434040;
                        border: 2px solid #e0e0e0;
                        border-radius: 8px;
                        color: #f8f5f5;
                        font-size: 18px;
                        padding: 10px;
                        width: 55px;
                        text-align: center;
                        outline: none;
                        transition: all 0.3s ease;
                        /* Esconde as setinhas laterais nativas do navegador */
                        -moz-appearance: textfield;
                      }

                      .input-numero::-webkit-outer-spin-button,
                      .input-numero::-webkit-inner-spin-button {
                        -webkit-appearance: none;
                        margin: 0;
                      }

                      .input-numero:focus {
                        background-color: #000000;
                        border-color: #0076ff;
                        box-shadow: 0 0 0 4px rgba(0, 118, 255, 0.15);
                      }
                    </style>

                    <div class="campo-hora">
                      <label style="font-weight: 600; color: #f8f5f5; font-family: 'Segoe UI', sans-serif;">
                        Horário de ativação automática
                      </label>

                      <div style="display: flex; align-items: center; justify-content: center; gap: 8px;">
                        <input type="number" id="input-hora" min="0" max="23" placeholder="00" class="input-numero">
                        <span style="color: white; font-size: 20px; font-weight: bold;">:</span>
                        <input type="number" id="input-minuto" min="0" max="59" placeholder="00" class="input-numero">
                      </div>

                      <button class="btn" style="background-color: #ff9800; margin-top: 15px;" onclick="atualizaHorario()">
                        Salvar Horário
                      </button>
                    </div>

                    <script>
                      // Função para enviar o horário escolhido pro ESP32
                      function atualizaHorario() {
                        const hora = document.getElementById('input-hora').value;
                        const minuto = document.getElementById('input-minuto').value;

                        // Proteção: não deixa enviar se estiver vazio
                        if (hora === "" || minuto === "") {
                          alert("Por favor, preencha a hora e o minuto!");
                          return; // Para a função aqui e não envia
                        }

                        // Manda a requisição pro garçom (fetch) com duas variáveis: h e m
                        // Exemplo: /agendar?h=14&m=30
                        fetch('/agendar?h=' + hora + '&m=' + minuto)
                          .then(response => {
                            alert("Horário programado para as " + hora + ":" + minuto + " com sucesso!");
                          });
                      }

                      // Trava de segurança para impedir o usuário de digitar horas e minutos inválidos
                      function limitarNumero(input, maximo) {
                        // 1. Apaga instantaneamente qualquer letra (incluindo 'e'), sinal ou ponto
                        input.value = input.value.replace(/\D/g, '');

                        if (input.value === "") return;

                        // 2. Converte para número real para verificar o limite
                        let valor = parseInt(input.value);
                        if (valor > maximo) input.value = maximo; // Impede digitar ex: 25h ou 70 min
                      }

                      document.getElementById('input-hora').addEventListener('input', function () {
                        limitarNumero(this, 23);
                      });
                      document.getElementById('input-minuto').addEventListener('input', function () {
                        limitarNumero(this, 59);
                      });
                    </script>
                  </div>
                </body>

                </html>
  )rawhtml");
            }
            break; 
          } else {
            linhaAtual = ""; 
          }
        } else if (c != '\r') {
          linhaAtual += c;
        }
      }
    }
    client.stop(); 
  }

  if (verificar_rfid()){
    sistemaLiberado = true;
    tempoInicio = millis();
    statusSite = "Sistema liberado";
    Serial.println("SISTEMA LIBERADO POR 10 SEGUNDOS!");
    bipCartao();
  }

  if ((digitalRead(botao) == LOW) && (sistemaLiberado)) {
    verde_vermelho *= -1;  
    delay(300);
  }

  if (verde_vermelho == -1) {
    digitalWrite(ledVerde, HIGH);
    digitalWrite(pinRele, HIGH);
    digitalWrite(ledVermelho, LOW);
  } else {
    digitalWrite(ledVerde, LOW);
    digitalWrite(pinRele, LOW);
    digitalWrite(ledVermelho, HIGH);
  }

  if (verde_vermelho != ultimo_verde_vermelho) {
    if (verde_vermelho == -1) {
      bipLigar();
    } else {
      bipDesligar();
    }
    ultimo_verde_vermelho = verde_vermelho;
  }

  //VERIFICACAO DO HORARIO NO LOOP
    if (millis() - ultimoTempoRelogio >= 1000) {
    ultimoTempoRelogio = millis();
    printLocalTime();
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      
      if (timeinfo.tm_hour == horaAgendada && 
           timeinfo.tm_min == minutoAgendado) { //07:30

        if (!jaAcionouAgendado) {
          jaAcionouAgendado = true;
          sistemaLiberado = true;
          tempoInicio = millis();
          verde_vermelho = -1; 

          statusSite = "Irrigação programada ativada";
          Serial.println("HORÁRIO AGENDADO (07:30) - IRRIGAÇÃO LIGADA!");
        }
      } else {
        jaAcionouAgendado = false; 
      }

    }
  }
  
  if (sistemaLiberado && (millis() - tempoInicio >= tempoMax)) {
    sistemaLiberado = false;
    statusSite = "Botao bloqueado (Tempo esgotado)";
    Serial.println("TEMPO ESGOTADO. Botao bloqueado.");
    verde_vermelho = 1;
    ultimo_verde_vermelho = 1; 
    bipDesligarTotal();
    rfid.PCD_SoftPowerDown();
    delay(500);
    rfid.PCD_SoftPowerUp();
  }
}