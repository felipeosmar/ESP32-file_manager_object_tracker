# ESP32 Object Tracker with Pan/Tilt Control

Sistema de rastreamento de objetos usando ESP32-CAM com câmera OV2640 e controle automático de servos Pan/Tilt.

## 📋 Características

- ✅ Streaming de vídeo em tempo real via web
- ✅ Detecção de movimento e rastreamento de objetos
- ✅ Controle automático de servos Pan/Tilt com PID
- ✅ Interface web responsiva armazenada em cartão SD
- ✅ Configuração via arquivo JSON no SD card
- ✅ Modo AP (Access Point) ou Station WiFi
- ✅ Controle manual via interface web
- ✅ Atalhos de teclado para controle rápido

## 🔧 Hardware Necessário

### Componentes Principais
- **ESP32-CAM** (ou ESP32 com módulo câmera OV2640)
- **Câmera OV2640** (640x480 VGA)
- **2x Servos** (SG90 ou similar, 0-180°)
- **Cartão microSD** (mínimo 1GB, formatado FAT32)
- **Fonte 5V** (mínimo 2A recomendado)

### Conexões

#### Servos
```
Servo Pan:  GPIO 12 (ajustável em camera_config.h)
Servo Tilt: GPIO 13 (ajustável em camera_config.h)
```

#### Câmera OV2640
As conexões da câmera seguem o padrão ESP32-CAM (AI-Thinker):
```
PWDN  = GPIO32    Y9 = GPIO35    VSYNC = GPIO25
RESET = -1        Y8 = GPIO34    HREF  = GPIO23
XCLK  = GPIO0     Y7 = GPIO39    PCLK  = GPIO22
SIOD  = GPIO26    Y6 = GPIO36
SIOC  = GPIO27    Y5 = GPIO21
                  Y4 = GPIO19
                  Y3 = GPIO18
                  Y2 = GPIO5
```

#### Cartão SD
O ESP32-CAM usa o modo SD_MMC (1-bit):
```
CMD   = GPIO15
CLK   = GPIO14
DATA0 = GPIO2
```

### Diagrama de Pinagem
```
ESP32-CAM
┌─────────────────┐
│  ┌─────────┐    │
│  │ OV2640  │    │──── Servo Pan (GPIO12)
│  │ Camera  │    │
│  └─────────┘    │──── Servo Tilt (GPIO13)
│                 │
│   [SD Card]     │──── 5V Power
│                 │
└─────────────────┘──── GND
```

## 📦 Software e Bibliotecas

### Dependências (instaladas via PlatformIO)
```ini
- ESP32Servo (v1.2.1+)
- ESPAsyncWebServer (v1.2.6+)
- AsyncTCP (v1.1.4+)
- ArduinoJson (v7.0.4+)
```

### Instalação

1. **Clone ou copie este projeto**
```bash
cd ESP32-object_tracker
```

2. **Prepare o cartão SD**
   - Formate o cartão SD como FAT32
   - Crie uma pasta chamada `web` na raiz do SD
   - Copie os arquivos da pasta `data/` para o SD:
     ```
     SD Card/
     ├── web/
     │   ├── index.html
     │   ├── style.css
     │   └── app.js
     └── config.json
     ```

3. **Configure WiFi** (edite `config.json` no SD card)
```json
{
  "wifi": {
    "ssid": "SuaRedeWiFi",
    "password": "SuaSenha",
    "ap_mode": false
  }
}
```

4. **Compile e faça upload**
```bash
pio run -t upload
```

5. **Monitor Serial** (opcional)
```bash
pio device monitor
```

## 🚀 Uso

### Primeira Inicialização

1. Insira o cartão SD no ESP32
2. Conecte a alimentação 5V
3. O ESP32 irá:
   - Inicializar o SD card
   - Carregar configurações do `config.json`
   - Inicializar a câmera
   - Configurar os servos (centralizar)
   - Conectar ao WiFi ou criar AP

### Acessando a Interface Web

**Modo AP (padrão):**
```
SSID: ESP32-Tracker
Senha: 12345678
URL: http://192.168.4.1
```

**Modo Station:**
```
Verifique o IP no Serial Monitor
URL: http://[IP-do-ESP32]
```

### Interface Web

A interface possui 4 seções principais:

#### 1. Visualização da Câmera
- Stream em tempo real
- Mira central (crosshair)
- Indicador de objeto rastreado

#### 2. Rastreamento Automático
- **Toggle ON/OFF**: Ativa/desativa rastreamento
- Quando ativo, os servos seguem automaticamente objetos em movimento

#### 3. Controle Manual
- **Joystick virtual**: Controle direcional
- **Botão Center**: Centraliza os servos (90°/90°)
- **Sliders Pan/Tilt**: Ajuste preciso (0-180°)

#### 4. Configurações
- **Sensibilidade**: Threshold de detecção (10-100)
- **Velocidade**: Rapidez do rastreamento (1-10)

### Atalhos de Teclado
```
↑ ↓ ← →  : Controle manual dos servos
C        : Centralizar servos
T        : Toggle rastreamento automático
```

## ⚙️ Configuração Avançada

### Arquivo config.json

```json
{
  "wifi": {
    "ssid": "MinhaRede",
    "password": "MinhasEnha",
    "ap_mode": false,
    "hostname": "esp32-tracker"
  },
  "camera": {
    "framesize": "VGA",
    "quality": 12,
    "brightness": 0,
    "contrast": 0,
    "saturation": 0
  },
  "servos": {
    "pan_pin": 12,
    "tilt_pin": 13,
    "pan_min": 0,
    "pan_max": 180,
    "tilt_min": 0,
    "tilt_max": 180,
    "pan_center": 90,
    "tilt_center": 90
  },
  "tracking": {
    "motion_threshold": 30,
    "speed": 5,
    "auto_enabled": true,
    "pid_p": 0.5,
    "pid_i": 0.0,
    "pid_d": 0.1
  },
  "system": {
    "serial_baud": 115200,
    "web_port": 80,
    "stream_fps": 10
  }
}
```

### Ajuste do PID

O controle dos servos usa um algoritmo PID. Para ajustar:

```cpp
servoController.setPIDGains(
  0.5,  // P - Proporcional (resposta imediata)
  0.0,  // I - Integral (correção de erro acumulado)
  0.1   // D - Derivativo (suavização)
);
```

**Dicas de ajuste:**
- **P alto**: Resposta rápida, mas pode oscilar
- **I**: Útil para eliminar erro residual (use com cuidado)
- **D alto**: Movimento mais suave, mas resposta lenta

## 📡 API REST

### Endpoints Disponíveis

#### GET /api/status
Retorna status atual do sistema
```json
{
  "tracking": true,
  "pan": 90,
  "tilt": 90,
  "motion_threshold": 30,
  "tracking_speed": 5
}
```

#### POST /api/tracking
Ativa/desativa rastreamento automático
```
Parâmetros: enabled=true|false
```

#### POST /api/center
Centraliza os servos (90°/90°)

#### POST /api/manual
Controle manual dos servos
```
Parâmetros: pan=0-180, tilt=0-180
```

#### GET /stream
Stream MJPEG da câmera
```
Content-Type: multipart/x-mixed-replace; boundary=frame
```

## 🔍 Detecção de Movimento

### Algoritmo

1. **Amostragem**: Frame reduzido para 80x60 pixels (performance)
2. **Diferenciação**: Comparação com frame anterior
3. **Threshold**: Pixels com diferença > threshold são marcados
4. **Centroid**: Cálculo do centro de massa dos pixels marcados
5. **PID**: Ajuste dos servos para centralizar o objeto

### Otimização

- Frame rate de detecção: ~10 FPS
- Update dos servos: 20 Hz
- Downsampling: 80x60 pixels para processamento

## 🛠️ Troubleshooting

### Problemas Comuns

**Câmera não inicializa**
```
- Verifique as conexões
- Certifique-se que está usando ESP32-CAM ou módulo compatível
- Ajuste pins em camera_config.h se necessário
```

**SD Card não detectado**
```
- Formate como FAT32
- Use cartão classe 10
- Verifique conexões SD_MMC
```

**Servos não respondem**
```
- Verifique alimentação (servos precisam de corrente adequada)
- Confirme GPIOs corretos em camera_config.h
- Teste com código simples de servo primeiro
```

**WiFi não conecta**
```
- Verifique SSID/senha no config.json
- Tente modo AP (ap_mode: true)
- Verifique força do sinal
```

**Stream lento ou travando**
```
- Reduza qualidade JPEG (aumentar número em config)
- Use WiFi mais próximo
- Evite múltiplas conexões simultâneas
```

### Debug via Serial

```cpp
// Habilite debug detalhado no platformio.ini
build_flags = -DCORE_DEBUG_LEVEL=5
```

## 📈 Melhorias Futuras

- [ ] Reconhecimento de objetos com TensorFlow Lite
- [ ] Gravação de vídeo no SD card
- [ ] Detecção de rostos
- [ ] Rastreamento multi-objeto
- [ ] OTA (Over-The-Air) updates
- [ ] Integração com MQTT/Home Assistant
- [ ] Modo noturno com LED IR
- [ ] Calibração automática de servos

## 📝 Estrutura do Projeto

```
ESP32-object_tracker/
├── ESP32-object_tracker.ino  # Código principal
├── camera_config.h            # Configuração de pinos
├── motion_detector.h/cpp      # Detecção de movimento
├── servo_controller.h/cpp     # Controle PID dos servos
├── sd_manager.h/cpp           # Gerenciamento SD card
├── web_server.h               # Declarações web server
├── platformio.ini             # Configuração PlatformIO
├── README.md                  # Esta documentação
└── data/                      # Arquivos para SD card
    ├── index.html
    ├── style.css
    ├── app.js
    └── config.json
```

## 📄 Licença

MIT License - Livre para uso pessoal e comercial

## 🤝 Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para:
- Reportar bugs
- Sugerir melhorias
- Enviar pull requests

## 📞 Suporte

Para dúvidas ou problemas:
1. Verifique a seção Troubleshooting
2. Revise o Serial Monitor para mensagens de erro
3. Abra uma issue no repositório

---

**Desenvolvido com ❤️ para a comunidade maker ESP32**

Versão: 1.0
Data: 2024
