# ESP32-CAM File Manager

Sistema completo de gerenciamento de arquivos e streaming de vídeo para ESP32-CAM com suporte a atualizações OTA (Over-The-Air).

## Características

- **Streaming de Vídeo em Tempo Real**: Stream MJPEG da câmera OV2640 via interface web
- **Gerenciador de Arquivos Completo**: Upload, download, edição, exclusão e visualização de arquivos no cartão SD
- **Atualizações OTA**: Sistema seguro de atualização de firmware over-the-air com validação e rollback automático
- **Monitor de Saúde do Sistema**: Dashboard completo com métricas de CPU, memória, WiFi e cartão SD
- **Interface Web Responsiva**: Interface moderna e intuitiva armazenada no cartão SD
- **Configuração via JSON**: Configuração de WiFi e sistema através de arquivo JSON no cartão SD
- **Modo AP e Station**: Suporta tanto Access Point quanto conexão a redes WiFi existentes

## Hardware Necessário

- **ESP32-CAM** (AI-Thinker ou similar)
- **Cartão MicroSD** (formatado em FAT32)
- **Programador USB-Serial** (FTDI ou CP2102) para upload inicial
- **Fonte de alimentação 5V** (mínimo 500mA recomendado)

### Pinout ESP32-CAM

O projeto utiliza os pinos padrão do módulo ESP32-CAM:

**Câmera OV2640:**
- Y2-Y9: GPIO5, GPIO18, GPIO19, GPIO21, GPIO36, GPIO39, GPIO34, GPIO35
- XCLK: GPIO0
- PCLK: GPIO22
- VSYNC: GPIO25
- HREF: GPIO23
- SDA: GPIO26
- SCL: GPIO27
- PWDN: GPIO32
- RESET: -1 (não usado)

**Cartão SD (modo 1-bit):**
- CLK: GPIO14
- CMD: GPIO15
- DATA0: GPIO2

## Instalação

### 1. Preparação do Ambiente

```bash
# Clone o repositório
git clone https://github.com/seu-usuario/ESP32-file_manager_object_tracker.git
cd ESP32-file_manager_object_tracker

# Instale o PlatformIO (se ainda não tiver)
pip install platformio

# Compile o projeto
pio run
```

### 2. Preparação do Cartão SD

1. Formate o cartão SD em **FAT32**
2. Crie a estrutura de diretórios:
```
/web/
  ├── index.html
  ├── style.css
  ├── app.js
  ├── filemanager.html
  ├── filemanager.css
  ├── filemanager.js
  ├── health.html
  ├── health.css
  ├── health.js
  ├── firmware.html
  ├── firmware.css
  └── firmware.js
/config.json (opcional)
```

3. Copie todos os arquivos da pasta `data/web/` para o diretório `/web/` do cartão SD

4. (Opcional) Crie o arquivo `config.json` na raiz do cartão SD:
```json
{
  "wifi": {
    "ssid": "SuaRedeWiFi",
    "password": "SuaSenha",
    "ap_mode": false
  }
}
```

**Nota:** Se o arquivo `config.json` não existir, o ESP32-CAM iniciará em modo Access Point com:
- SSID: `ESP32-CAM`
- Senha: `12345678`

### 3. Upload do Firmware

```bash
# Conecte o programador USB-Serial ao ESP32-CAM
# GND -> GND
# 5V -> 5V
# U0R (RX) -> TX do programador
# U0T (TX) -> RX do programador
# GPIO0 -> GND (para entrar em modo flash)

# Faça o upload
pio run --target upload

# Remova o jumper GPIO0->GND e pressione o botão RESET
```

## Uso

### Primeira Conexão

1. **Modo AP (padrão sem config.json):**
   - Conecte-se à rede WiFi `ESP32-CAM` (senha: `12345678`)
   - Acesse: `http://192.168.4.1`

2. **Modo Station (com config.json):**
   - O ESP32 conectará à rede configurada
   - Verifique o IP no Serial Monitor (115200 baud)
   - Acesse: `http://[IP_DO_ESP32]`

### Páginas Disponíveis

- **`/`** - Página principal com stream de vídeo
- **`/filemanager`** - Gerenciador de arquivos
- **`/health`** - Monitor de saúde do sistema
- **`/firmware`** - Atualização de firmware OTA

### API Endpoints

#### Camera
- `GET /stream` - Stream MJPEG da câmera

#### Arquivos
- `GET /api/files/list?dir=/path` - Lista arquivos em um diretório
- `GET /api/files/download?file=/path/file` - Baixa um arquivo
- `GET /api/files/view?file=/path/file` - Visualiza conteúdo do arquivo
- `GET /api/files/read?file=/path/file` - Lê arquivo para edição (máx 50KB)
- `POST /api/files/write` - Salva arquivo editado
- `POST /api/files/upload?dir=/path` - Upload de arquivo
- `POST /api/files/delete` - Deleta arquivo/diretório
- `POST /api/files/mkdir` - Cria diretório

#### Sistema
- `GET /api/health/status` - Status completo do sistema

#### Firmware
- `POST /api/firmware/upload` - Upload de novo firmware (.bin)

## Atualização OTA (Over-The-Air)

### Como Atualizar o Firmware

1. Compile o novo firmware:
```bash
pio run
# O arquivo .bin estará em: .pio/build/esp32cam/firmware.bin
```

2. Acesse a página de firmware: `http://[IP_DO_ESP32]/firmware`

3. Selecione o arquivo `.bin` e faça o upload

4. O ESP32 irá:
   - Validar o firmware (verifica magic byte 0xE9)
   - Desativar a câmera temporariamente
   - Gravar o novo firmware na partição OTA
   - Reiniciar automaticamente

5. Na primeira requisição HTTP após o boot, o sistema valida a partição OTA e cancela o rollback automático

### Segurança OTA

- **Validação de Firmware**: Verifica se o arquivo é um binário ESP32 válido
- **Rollback Automático**: Se o novo firmware não responder em ~60 segundos, o bootloader retorna à versão anterior
- **Mutex de Proteção**: Bloqueia operações do cartão SD durante a atualização
- **Desativação da Câmera**: Libera pinos compartilhados durante a gravação

### Particionamento

O projeto usa o esquema `min_spiffs.csv`:
- **app0 (ota_0)**: 1.9MB - Primeira partição OTA
- **app1 (ota_1)**: 1.9MB - Segunda partição OTA
- **spiffs**: 190KB - Sistema de arquivos (não utilizado, SD card é usado)

## Arquitetura do Sistema

### Estrutura de Código

```
src/
├── main.cpp          # Loop principal e configuração do servidor
├── camera_config.h   # Configuração de pinos da câmera
├── sd_manager.h/cpp  # Gerenciamento do cartão SD
└── web_server.h      # Definições do servidor web

data/web/
├── index.html        # Página principal com stream
├── style.css         # Estilos globais
├── app.js           # JavaScript do stream
├── filemanager.*    # Gerenciador de arquivos
├── health.*         # Monitor de saúde
└── firmware.*       # Interface de atualização OTA
```

### Fluxo de Inicialização

1. **Inicialização do Serial** (115200 baud)
2. **Criação do Mutex** para controle de acesso ao SD
3. **Inicialização do SD Card** (modo 1-bit)
4. **Carregamento da Configuração** (config.json)
5. **Inicialização da Câmera** (OV2640, VGA, JPEG)
6. **Configuração WiFi** (AP ou Station)
7. **Inicialização do Servidor Web** (porta 80)
8. **Sistema Pronto**

### Gerenciamento de Recursos

- **Mutex para SD Card**: Previne acessos concorrentes durante operações críticas (OTA, leitura/escrita)
- **Controle de Câmera**: Flag `cameraActive` para pausar câmera durante OTA
- **Watchdog**: Delays estratégicos para alimentar o watchdog durante operações longas
- **Frame Rate**: Limitado a ~10 FPS para estabilidade
- **Buffer de Upload**: Operações em chunks para gerenciar memória

## Monitor de Saúde

O endpoint `/api/health/status` retorna informações completas:

```json
{
  "uptime": {
    "milliseconds": 123456,
    "formatted": "0d 0h 2m 3s"
  },
  "memory": {
    "heap": {
      "total": 327680,
      "free": 250000,
      "used": 77680,
      "usage_percent": 23.7
    },
    "psram": {
      "total": 4194304,
      "free": 4000000,
      "used": 194304,
      "usage_percent": 4.6
    }
  },
  "wifi": {
    "connected": true,
    "ssid": "MinhaRede",
    "rssi": -65,
    "signal_strength": "Good",
    "ip": "192.168.1.100",
    "mac": "AA:BB:CC:DD:EE:FF",
    "channel": 6
  },
  "sd_card": {
    "ready": true,
    "card_size_mb": 7580,
    "total_mb": 7456,
    "used_mb": 1234,
    "free_mb": 6222,
    "usage_percent": 16.5,
    "type": "SDHC"
  },
  "cpu": {
    "frequency_mhz": 240,
    "cores": 2,
    "chip_model": "ESP32-D0WDQ6",
    "chip_revision": 1,
    "sdk_version": "v4.4.6"
  },
  "flash": {
    "size_mb": 4,
    "speed_mhz": 80
  },
  "ota": {
    "upload_in_progress": false
  },
  "status": "healthy",
  "timestamp": 123456
}
```

## Configuração da Câmera

Configurações padrão (ajustáveis em `src/main.cpp`):

- **Frame Size**: VGA (640x480)
- **Qualidade JPEG**: 12 (0-63, menor = melhor qualidade)
- **Frame Buffer Count**: 2
- **Formato**: JPEG
- **Frequência XCLK**: 20MHz

Para alterar a resolução, modifique em `main.cpp:142`:
```cpp
config.frame_size = FRAMESIZE_QVGA;  // 320x240
config.frame_size = FRAMESIZE_VGA;   // 640x480 (padrão)
config.frame_size = FRAMESIZE_SVGA;  // 800x600
config.frame_size = FRAMESIZE_XGA;   // 1024x768
```

## Dependências

Definidas em `platformio.ini`:

- **Platform**: espressif32
- **Framework**: Arduino
- **Bibliotecas**:
  - ESPAsyncWebServer (async web server)
  - AsyncTCP (TCP assíncrono)
  - ArduinoJson 7.0.4 (manipulação JSON)

## Solução de Problemas

### Cartão SD não detectado
- Verifique se o cartão está formatado em FAT32
- Teste com cartão de menor capacidade (≤32GB funciona melhor)
- Verifique conexões físicas

### Câmera não inicializa
- Erro comum com módulos AI-Thinker
- Verifique alimentação (mínimo 500mA)
- Adicione capacitor 100-470µF entre 5V e GND

### WiFi não conecta
- Verifique SSID e senha no `config.json`
- Força do sinal fraco: aproxime o ESP32 do roteador
- Se falhar, entrará em modo AP automaticamente

### OTA falha
- Verifique se o arquivo é `.bin` válido
- Arquivo muito grande: máximo ~1.9MB
- Durante upload, evite operações no SD

### Stream lento/travando
- Reduza resolução (QVGA = 320x240)
- Aumente qualidade JPEG (valor maior = menor qualidade = arquivo menor)
- Verifique força do sinal WiFi

## Desenvolvimento

### Compilar e Monitorar

```bash
# Compilar
pio run

# Upload
pio run --target upload

# Monitor serial
pio device monitor

# Compilar + Upload + Monitor
pio run --target upload && pio device monitor
```

### Debug

O nível de debug está configurado em `platformio.ini`:
```ini
build_flags =
    -DCORE_DEBUG_LEVEL=3  # 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose
```

### Modificar Interface Web

1. Edite os arquivos em `data/web/`
2. Copie os arquivos modificados para o cartão SD
3. Reinicie o ESP32 ou atualize a página (Ctrl+F5)

## Especificações Técnicas

- **MCU**: ESP32-D0WDQ6 (dual-core 240MHz)
- **Câmera**: OV2640 (2MP)
- **RAM**: 520KB SRAM + 4MB PSRAM
- **Flash**: 4MB
- **WiFi**: 802.11 b/g/n (2.4GHz)
- **Protocolo Stream**: MJPEG over HTTP
- **Servidor Web**: Assíncrono (não bloqueante)

## Consumo de Recursos

- **Memória Heap**: ~250KB livre (de 320KB)
- **PSRAM**: ~4MB livre (usado principalmente para buffers de câmera)
- **Taxa de Transferência**: ~10 FPS @ VGA
- **Tamanho do Firmware**: ~1.2MB

## Licença

Este projeto é fornecido como está, para fins educacionais e de desenvolvimento.

## Autor

Desenvolvido com ESP32-CAM, Arduino Framework e PlatformIO.

## Referências

- [ESP32-CAM Datasheet](Datasheet%20ESP32-CAM.pdf)
- [ESP32 Datasheet](ESP32-D0WDQ6-V3.PDF)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer)

## Roadmap / Melhorias Futuras

- [ ] Autenticação de usuário
- [ ] HTTPS/SSL
- [ ] Gravação de vídeo no SD
- [ ] Detecção de movimento
- [ ] Notificações push
- [ ] Suporte MQTT
- [ ] Time-lapse automático
- [ ] Múltiplos streams simultâneos

## Contribuindo

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues ou pull requests.

---

**Nota:** Este projeto foi desenvolvido e testado com módulos ESP32-CAM AI-Thinker. Outros modelos podem requerer ajustes nos pinos e configurações.
