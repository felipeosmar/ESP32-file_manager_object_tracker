# Guia de Instalação - ESP32 Object Tracker

O PlatformIO foi atualizado com sucesso, mas há problemas com o download do framework. Aqui estão **3 alternativas** para compilar o projeto:

## Opção 1: Usar Arduino IDE (Mais Simples)

### Passo 1: Instalar Arduino IDE 2.x

```bash
# Download do site oficial
https://www.arduino.cc/en/software

# Ou via snap (Ubuntu/Debian)
sudo snap install arduino
```

### Passo 2: Adicionar ESP32 ao Arduino IDE

1. Abra Arduino IDE
2. Vá em **File > Preferences**
3. Em "Additional Boards Manager URLs", adicione:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Vá em **Tools > Board > Boards Manager**
5. Busque por "esp32" e instale "esp32 by Espressif Systems"

### Passo 3: Instalar Bibliotecas

No Arduino IDE, vá em **Sketch > Include Library > Manage Libraries** e instale:

- **ESP32Servo** (by Kevin Harrington)
- **ESPAsyncWebServer** (by lacamera or me-no-dev)
- **AsyncTCP** (by me-no-dev)
- **ArduinoJson** (by Benoit Blanchon) versão 7.x

### Passo 4: Configurar e Compilar

1. Abra o arquivo `ESP32-object_tracker.ino` no Arduino IDE
2. Vá em **Tools** e configure:
   - **Board**: "AI Thinker ESP32-CAM"
   - **Port**: Selecione a porta USB do seu ESP32
   - **Partition Scheme**: "Huge APP (3MB No OTA/1MB SPIFFS)"
   - **Upload Speed**: 921600
3. Clique em **Verify** (✓) para compilar
4. Clique em **Upload** (→) para enviar ao ESP32

---

## Opção 2: PlatformIO com Download Manual

Se o download automático estiver falhando, você pode baixar manualmente:

### Passo 1: Baixar framework manualmente

```bash
# Criar diretório para frameworks
mkdir -p ~/.platformio/packages

# Baixar framework Arduino ESP32 (pode demorar)
wget https://github.com/platformio/platform-espressif32/releases/download/v6.0.0/framework-arduinoespressif32.tar.gz -O /tmp/arduino-esp32.tar.gz

# Extrair
tar -xzf /tmp/arduino-esp32.tar.gz -C ~/.platformio/packages/
```

### Passo 2: Tentar compilar novamente

```bash
cd /home/felipe/work/ESP32-object_tracker
pio run
```

---

## Opção 3: PlatformIO com Timeout Aumentado

Às vezes o download é lento. Tente com configurações diferentes:

### Editar platformio.ini

Adicione ao arquivo `platformio.ini`:

```ini
[platformio]
; Aumentar timeout de download
extra_script = pre:increase_timeout.py
```

### Criar script de timeout

Crie o arquivo `increase_timeout.py`:

```python
Import('env')
import os
os.environ['PLATFORMIO_DOWNLOAD_TIMEOUT'] = '600'
```

### Tentar novamente

```bash
pio run
```

---

## Opção 4: Usar Versão Simplificada (Sem AsyncWebServer)

Se continuar com problemas de bibliotecas, podemos criar uma versão simplificada que usa apenas bibliotecas core do ESP32.

---

## Verificação de Problemas Comuns

### Problema: Erro de permissão na porta USB

```bash
# Adicionar usuário ao grupo dialout
sudo usermod -a -G dialout $USER

# Reiniciar ou fazer logout/login
```

### Problema: ESP32 não entra em modo de programação

1. Mantenha pressionado o botão **GPIO 0** (ou **IO0** ou **BOOT**)
2. Pressione e solte o botão **RESET** (ou **RST**)
3. Solte o botão GPIO 0
4. Tente fazer upload novamente

### Problema: "Timeout waiting for packet header"

1. Reduza a velocidade de upload para 115200
2. Use um cabo USB de melhor qualidade
3. Tente uma porta USB diferente

---

## Estrutura de Arquivos para SD Card

Antes de testar, prepare o cartão SD:

```
SD Card (formatado como FAT32)
│
├── web/
│   ├── index.html
│   ├── style.css
│   └── app.js
│
└── config.json
```

Copie os arquivos da pasta `data/` para o SD:

```bash
# Assumindo que o SD está montado em /media/sdcard
cp -r data/index.html data/style.css data/app.js /media/sdcard/web/
cp data/config.json /media/sdcard/
```

---

## Teste Rápido (Sem SD Card)

Se quiser testar primeiro sem o SD card:

1. O código tem um HTML embutido (fallback)
2. Funciona sem SD, mas com interface simplificada
3. Configurações usarão valores padrão (AP mode)

---

## Próximos Passos Após Compilar

1. **Conecte o hardware**: Servos nos GPIOs 12 e 13
2. **Insira o SD card**: Com os arquivos web
3. **Alimente o ESP32**: 5V/2A mínimo
4. **Monitore o Serial**: Para ver o IP
   ```bash
   # PlatformIO
   pio device monitor

   # Arduino IDE
   Tools > Serial Monitor (115200 baud)
   ```
5. **Acesse a interface**: http://192.168.4.1 (modo AP)

---

## Suporte

Se continuar com problemas:

1. Verifique a versão do Python: `python3 --version` (recomendado: 3.8-3.11)
2. Verifique o PlatformIO: `pio --version` (deve ser 6.x)
3. Tente limpar cache: `pio system prune -f`
4. Reinstale PlatformIO: `pip3 uninstall platformio && pip3 install platformio`

---

**Recomendação**: Use a **Opção 1 (Arduino IDE)** para começar rapidamente!
