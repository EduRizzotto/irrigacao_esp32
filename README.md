# 💧 Sistema de Irrigação Controlado com ESP32 (RFID + WiFi)

Este projeto é um sistema inteligente de controle de irrigação utilizando uma placa **ESP32**. Ele funciona como um "controle de acesso" para uma bomba d'água, onde o uso só é liberado mediante aproximação de um **Cartão RFID** autorizado ou através de um painel de controle **Web (WiFi)** via celular ou computador.

## 🚀 Funcionalidades

- **Liberação por RFID:** Utiliza o leitor MFRC522 para liberar o uso aproximando uma tag ou cartão cadastrado.
- **Painel de Controle Remoto:** O ESP32 atua como um Servidor Web. Acessando o IP da placa pelo navegador, você visualiza o status do sistema e pode "Liberar Remotamente".
- **Acionamento Manual Seguro:** O botão físico só tem efeito se o sistema for previamente "Liberado" pelo cartão ou pelo celular.
- **Temporizador Automático:** Uma vez liberado, o usuário tem 15 segundos de acesso. Se o tempo esgotar, a irrigação é bloqueada automaticamente.
- **Feedback Visual e Sonoro:** 
  - **LEDs** indicam se o sistema está ligado (Verde) ou bloqueado (Vermelho).
  - **Buzzer** emite bipes diferentes para alertar leitura de cartão, ligamento, desligamento e tempo esgotado.

## 🛠️ Hardware Utilizado

- Placa **ESP32** (modelo *esp32doit-devkit-v1*)
- Módulo **Leitor RFID MFRC522**
- Módulo **Relé** (para acionar a bomba de irrigação)
- 1 **Botão** (Push Button)
- 1 **Buzzer** 
- 2 **LEDs** (Verde e Vermelho)

## 📌 Esquema de Pinos (Pinout)

| Componente | Pino do ESP32 |
|------------|--------------|
| RFID (SDA/SS) | Pino 5 |
| RFID (RST) | Pino 22 |
| LED Verde | Pino 12 |
| LED Vermelho | Pino 13 |
| Botão Físico | Pino 14 |
| Módulo Relé | Pino 26 |
| Buzzer | Pino 27 |

*(O leitor RFID também utiliza os pinos padrão de hardware SPI do ESP32 para MOSI, MISO e SCK).*

## 💻 Software e Dependências

O projeto foi construído usando o **PlatformIO** (VS Code) em C++ (Framework Arduino). 
Dependências listadas no `platformio.ini`:
- `miguelbalboa/MFRC522` (Controle do leitor RFID)
- Biblioteca nativa de `WiFi` do ESP32.

## ⚙️ Como Utilizar

1. Abra a pasta do projeto no VS Code com a extensão **PlatformIO** instalada.
2. Certifique-se de configurar o nome e senha da sua rede WiFi no código (se aplicável).
3. Conecte seu ESP32 via USB.
4. Clique em **Upload** no painel do PlatformIO.
5. Abra o **Serial Monitor** (baud rate `115200`). Lá aparecerá o **endereço de IP** da placa.
6. Digite esse IP no navegador do seu celular ou computador para acessar a interface web de controle.

