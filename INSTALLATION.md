# Guia de Instalação - ESP32 Object Tracker

## 📋 Requisitos

### Hardware
- ESP32-CAM ou ESP32 com módulo de câmera OV2640
- 2x Servos motores (pan e tilt)
- Cartão microSD (1GB - 32GB, formatado em FAT32, Class 10 recomendado)
- Fonte de alimentação 5V
- Cabos jumper

### Software
- PlatformIO IDE ou Arduino IDE
- Driver CH340/CP2102 (para upload via USB)
- Navegador web moderno

## 🔧 Instalação

### Passo 1: Preparar o Ambiente

#### Usando PlatformIO (Recomendado)

```bash
# Clonar ou baixar o projeto
cd ESP32-object_tracker

# PlatformIO irá instalar as dependências automaticamente
pio lib install
```

#### Usando Arduino IDE

1. Instale as seguintes bibliotecas via Library Manager:
   - ESP32Servo (v1.2.1 ou superior)
   - ESPAsyncWebServer
   - AsyncTCP
   - ArduinoJson (v7.0.4 ou superior)

2. Configure a placa:
   - Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM

### Passo 2: Preparar o Cartão SD

1. **Formatar o cartão SD:**
   - Sistema de arquivos: **FAT32**
   - Tamanho de alocação: padrão

2. **Copiar arquivos para o SD:**
   ```
   Copiar todo o conteúdo da pasta data/ para a raiz do cartão SD
   ```

   Estrutura final no SD:
   ```
   /
   ├── config.json
   └── web/
       ├── index.html (opcional)
       ├── style.css (opcional)
       ├── app.js (opcional)
       ├── filemanager.html
       ├── filemanager.css
       └── filemanager.js
   ```

3. **Editar config.json:**
   ```json
   {
     "wifi": {
       "ssid": "Sua-Rede-WiFi",
       "password": "sua-senha-wifi",
       "ap_mode": false
     },
     "tracking": {
       "motion_threshold": 30,
       "speed": 5,
       "auto_enabled": true
     }
   }
   ```

   **Modo AP (Ponto de Acesso):** Use `"ap_mode": true` se quiser que o ESP32 crie sua própria rede WiFi.

### Passo 3: Conectar o Hardware

#### Conexões ESP32-CAM

**Câmera OV2640:** Já integrada no módulo ESP32-CAM

**Servos:**
- Servo PAN (horizontal):
  - Sinal: GPIO 13
  - VCC: 5V
  - GND: GND

- Servo TILT (vertical):
  - Sinal: GPIO 15
  - VCC: 5V
  - GND: GND

**Cartão SD:**
- O ESP32-CAM já possui slot SD integrado
- Modo de operação: 1-bit (para evitar conflito com servos)

**Alimentação:**
- 5V e GND da fonte

⚠️ **IMPORTANTE:** Desconecte os servos durante o upload do código!

### Passo 4: Compilar e Fazer Upload

#### PlatformIO

```bash
# Compilar
pio run

# Upload (ESP32-CAM desconectado dos servos)
pio run --target upload

# Monitor Serial
pio device monitor
```

#### Arduino IDE

1. Selecione a porta COM correta
2. Configure:
   - Upload Speed: 921600
   - Flash Frequency: 80MHz
   - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
3. Clique em Upload

### Passo 5: Primeiro Boot

1. **Insira o cartão SD** no ESP32-CAM
2. **Conecte os servos** nas posições corretas
3. **Alimente o ESP32** (5V)
4. **Abra o Serial Monitor** (115200 baud)

Você verá:
```
=== ESP32 Object Tracker ===
Initializing SD card...
SD Card initialized successfully
SD Card Type: SDHC
Camera initialized successfully
Servos initialized and centered
WiFi connected!
IP Address: 192.168.1.100
=== System Ready ===
Camera stream: http://192.168.1.100/
```

## 🌐 Acessar a Interface Web

### Modo Station (Conectado à Rede)
1. Anote o IP exibido no Serial Monitor
2. Abra o navegador
3. Acesse: `http://<IP_DO_ESP32>/`

### Modo AP (Ponto de Acesso)
1. Conecte-se à rede WiFi criada pelo ESP32
   - SSID: Conforme configurado (padrão: "ESP32-Tracker")
   - Senha: Conforme configurado (padrão: "12345678")
2. Acesse: `http://192.168.4.1/`

## 🎯 Usar o Sistema

### Página Principal (`/`)
- Visualização do stream da câmera em tempo real
- Controles de tracking automático
- Controle manual de pan/tilt
- Botão para centralizar servos

### File Manager (`/filemanager`)
- Navegação de arquivos do SD
- Upload de arquivos (drag & drop)
- Download de arquivos
- Visualização de arquivos de texto
- Criar/deletar pastas
- Deletar arquivos

### Endpoints da API

**Status:**
```bash
curl http://<IP>/api/status
```

**Ativar/Desativar Tracking:**
```bash
curl -X POST http://<IP>/api/tracking -d "enabled=true"
```

**Centralizar Servos:**
```bash
curl -X POST http://<IP>/api/center
```

**Controle Manual:**
```bash
curl -X POST http://<IP>/api/manual -d "pan=90&tilt=45"
```

**Listar Arquivos:**
```bash
curl http://<IP>/api/files/list?dir=/
```

**Upload de Arquivo:**
```bash
curl -X POST -F "file=@arquivo.txt" -F "dir=/" http://<IP>/api/files/upload
```

**Download de Arquivo:**
```bash
curl "http://<IP>/api/files/download?file=/arquivo.txt" -O
```

**Deletar Arquivo:**
```bash
curl -X POST http://<IP>/api/files/delete -d "file=/arquivo.txt"
```

**Criar Pasta:**
```bash
curl -X POST http://<IP>/api/files/mkdir -d "dir=/nova_pasta"
```

## ⚙️ Configurações Avançadas

### Ajustar Sensibilidade de Detecção

Edite `config.json` no SD card:
- `motion_threshold`: 10-100 (menor = mais sensível)

### Ajustar Velocidade dos Servos

Edite `config.json` no SD card:
- `speed`: 1-10 (maior = mais rápido)

### Mudar Resolução da Câmera

Edite `main.cpp` linha 168:
```cpp
config.frame_size = FRAMESIZE_VGA; // 640x480
```

Opções: QVGA (320x240), VGA (640x480), SVGA (800x600)

## 🐛 Solução de Problemas

### SD Card não reconhecido
- Verifique se está formatado em FAT32
- Tente um cartão diferente (máx 32GB)
- Verifique se os arquivos foram copiados corretamente
- Consulte Serial Monitor para mensagens de erro

### Camera init failed
- Verifique conexões da câmera
- Reinicie o ESP32
- Tente reduzir a resolução

### Servos não se movem
- Verifique alimentação (servos precisam de 5V adequado)
- Confirme conexões GPIO 13 e 15
- Verifique Serial Monitor para erros

### WiFi não conecta
- Verifique SSID e senha no config.json
- Tente modo AP (`"ap_mode": true`)
- Verifique Serial Monitor para detalhes

### Handler did not handle the request
- ✅ **RESOLVIDO:** Atualize para a versão mais recente do código
- Os arquivos HTML/CSS/JS agora são servidos do SD card
- Verifique se os arquivos estão no SD em `/web/`

### Interface não carrega CSS/JS
- Verifique estrutura de pastas no SD:
  - `/web/filemanager.html`
  - `/web/filemanager.css`
  - `/web/filemanager.js`
- Acesse endpoints diretamente:
  - `http://<IP>/filemanager.css`
  - `http://<IP>/filemanager.js`

### Falta de memória / Crashes
- ✅ **MELHORADO:** HTML embutido foi removido
- Use partition scheme "Huge APP"
- Reduza resolução da câmera se necessário

## 📊 Uso de Memória

### Antes (HTML embutido):
- Flash: ~85% utilizado
- RAM: ~70% utilizado

### Depois (HTML no SD):
- Flash: ~60% utilizado (-25%)
- RAM: ~65% utilizado (-5%)
- File Manager funcional sem ocupar memória flash!

## 📝 Notas

- O sistema funciona sem SD card, mas com funcionalidade limitada
- File Manager requer SD card
- Configurações padrão são usadas se config.json não existir
- Stream MJPEG pode ter latência dependendo da rede WiFi

## 🔐 Segurança

⚠️ **Este é um projeto de demonstração:**
- Não há autenticação na interface web
- Use apenas em redes privadas/confiáveis
- Não exponha diretamente à Internet sem proteção adicional

## 📄 Licença

Este projeto é open source. Consulte LICENSE para detalhes.
