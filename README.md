# 📈 Datalogger de Movimento com IMU – BitDogLab RP2040

Projeto embarcado desenvolvido para registro de movimentos utilizando o sensor **MPU6050**, com armazenamento em **cartão SD**, exibição de status em **display OLED SSD1306**, alertas via **LED RGB** e controle por **botões físicos**, com base na plataforma **BitDogLab** com **RP2040**.

## 📋 Descrição

Este sistema embarcado atua como um **registrador de dados de movimento (Datalogger)**. Ele captura continuamente dados de aceleração e rotação (giroscópio) em três eixos, grava essas informações em um arquivo `.csv` no cartão SD e fornece **feedback em tempo real** ao usuário via display e LEDs.

Funcionalidades principais:

- Leitura contínua dos sensores de **aceleração** e **giroscópio** (eixos X, Y, Z).
- Armazenamento estruturado dos dados no cartão SD.
- Indicação de estados operacionais com **LED RGB**, **Display OLED** e **buzzer**.
- Controle de captura e montagem/desmontagem do SD via **botões físicos**.

## ⚙️ Funcionalidades

- 📥 Registro de dados de movimento no formato `.csv`.
- 🧭 Captura de aceleração e giroscópio usando o sensor MPU6050.
- 💾 Criação automática do arquivo com cabeçalho e retomada a partir da última amostra.
- 🟢 LED verde: Sistema pronto  
- 🔴 LED vermelho: Captura em andamento  
- 🔵 LED azul piscando: Escrita no cartão SD  
- 🟣 LED roxo piscando: Erro  
- 📟 Display OLED: Status em tempo real
- 🔘 Botões físicos com interrupção e debounce para controle de captura e montagem do SD.

## 🧩 Componentes Utilizados

### 🔌 Hardware
- **Plataforma:** BitDogLab com RP2040
- **Sensor MPU6050:** Acelerômetro e Giroscópio via I2C
- **Display OLED SSD1306**
- **Cartão MicroSD com adaptador SPI**
- **LEDs (vermelho, verde, azul - RGB discreto)**
- **Buzzer**
- **Botões físicos:**  
  - Botão A: Inicia/para a captura  
  - Botão B: Monta/desmonta o cartão SD  
  - Joystick (Botão C): Entra no modo BOOTSEL

### 🛠️ Software
- **Linguagem:** C com FreeRTOS
- **Bibliotecas:**
  - `pico/stdlib`, `hardware/i2c`, `hardware/gpio`, `FreeRTOS`, `ff.h`
  - `ssd1306.h`, `mpu6050.h`, `hw_config.h`, `sd_card.h`, `f_util.h`

## 🚀 Como Usar

### 🔧 Instalação e Configuração

1. Clone o repositório:
   ```bash
   git clone https://github.com/seu-usuario/datalogger-movimento.git
   ```

2. Abra o projeto em um ambiente com suporte ao SDK do RP2040 (como VSCode + CMake).

3. Conecte os pinos conforme abaixo:

   - **I2C0 (MPU6050)**: SDA = GP0, SCL = GP1
   - **I2C1 (Display OLED SSD1306)**: SDA = GP14, SCL = GP15 
   - **Botão A:** GP5  
   - **Botão B:** GP6  
   - **Joystick botão:** GP22  
   - **LEDs RGB:** GP11 (verde), GP12 (azul), GP13 (vermelho)  
   - **Cartão SD:** Conforme hardware BitDogLab (via SPI)

4. Compile e carregue o firmware para a placa BitDogLab RP2040.

## 📂 Arquivo CSV Gerado

- Nome: `dados.csv`
- Formato:
  ```
  numero_amostra;accel_x;accel_y;accel_z;giro_x;giro_y;giro_z
  0;0.01;0.02;0.98;1.5;0.0;-0.1
  ...
  ```

- O número da amostra é contínuo, mesmo após reinicializações (lido da última linha do arquivo existente).

## 📈 Análise com Python

Um script em Python (`plot_dados.py`) pode ser utilizado para ler o CSV e gerar gráficos dos dados de aceleração e giroscópio ao longo do tempo.

## 📌 Observações

- O sistema trata debounce por software e interrupções por hardware para maior responsividade.
- A gravação no cartão SD é segura, com lógica de montagem/desmontagem controlada.
- A interface com o usuário é intuitiva e totalmente embarcada.

## 👨‍💻 Autor

**Matheus Nepomuceno Souza**  
BitDogLab | Projeto Individual - Datalogger de Movimento
