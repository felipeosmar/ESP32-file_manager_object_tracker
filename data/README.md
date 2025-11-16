# Estrutura de Arquivos do Cartão SD

Este diretório contém os arquivos que devem ser copiados para o **cartão SD** do ESP32.

## 📁 Estrutura Requerida

Copie todos os arquivos deste diretório para o cartão SD, mantendo a seguinte estrutura:

```
SD Card Root (/)
├── config.json                  # Configuração do sistema
└── web/                         # Arquivos da interface web
    ├── index.html              # Página principal (opcional)
    ├── style.css               # Estilos da página principal (opcional)
    ├── app.js                  # JavaScript da página principal (opcional)
    ├── filemanager.html        # Interface do gerenciador de arquivos
    ├── filemanager.css         # Estilos do gerenciador
    └── filemanager.js          # JavaScript do gerenciador
```

## 🔧 Configuração

### 1. Preparar o Cartão SD

- Formate o cartão SD em **FAT32**
- Tamanho recomendado: 1GB a 32GB
- Velocidade: Class 10 ou superior recomendado

### 2. Copiar Arquivos

Copie todos os arquivos e pastas deste diretório (`data/`) para a raiz do cartão SD:

```bash
# No Linux/Mac
cp -r data/* /path/to/sdcard/

# No Windows
# Use o Explorer para copiar os arquivos
```

### 3. Inserir no ESP32

- Desligue o ESP32
- Insira o cartão SD no slot SD_MMC
- Ligue o ESP32

## 📝 Arquivo config.json

O arquivo `config.json` contém as configurações do sistema. Exemplo:

```json
{
  "wifi": {
    "ssid": "Sua-Rede-WiFi",
    "password": "sua-senha",
    "ap_mode": false
  },
  "tracking": {
    "motion_threshold": 30,
    "speed": 5,
    "auto_enabled": true
  }
}
```

### Parâmetros WiFi

- `ssid`: Nome da rede WiFi
- `password`: Senha da rede WiFi
- `ap_mode`:
  - `false` = Conecta em rede WiFi existente (modo Station)
  - `true` = Cria ponto de acesso próprio (modo AP)

### Parâmetros de Tracking

- `motion_threshold`: Sensibilidade de detecção de movimento (10-100)
  - Valores menores = mais sensível
  - Valores maiores = menos sensível
- `speed`: Velocidade de movimento dos servos (1-10)
  - 1 = muito lento
  - 10 = muito rápido
- `auto_enabled`: Ativa/desativa tracking automático ao iniciar

## 🌐 Acessando o Sistema

### Modo Station (conectado à rede WiFi)

1. Conecte-se à mesma rede WiFi configurada
2. Verifique o IP no Serial Monitor
3. Acesse: `http://<IP_DO_ESP32>/`

### Modo AP (ponto de acesso)

1. Conecte-se à rede WiFi criada pelo ESP32
   - SSID: Conforme configurado em `config.json`
   - Senha: Conforme configurado
2. Acesse: `http://192.168.4.1/`

## 🛠️ Endpoints Disponíveis

### Interface Web

- `/` - Página principal com stream da câmera
- `/filemanager` - Gerenciador de arquivos do SD

### API Endpoints

**Status e Controle:**
- `GET /api/status` - Status do sistema
- `POST /api/tracking` - Ativar/desativar tracking
- `POST /api/center` - Centralizar servos
- `POST /api/manual` - Controle manual dos servos

**Gerenciamento de Arquivos:**
- `GET /api/files/list?dir=/path` - Listar arquivos
- `GET /api/files/download?file=/path/file` - Baixar arquivo
- `GET /api/files/view?file=/path/file` - Visualizar arquivo
- `POST /api/files/upload` - Upload de arquivo
- `POST /api/files/delete` - Deletar arquivo
- `POST /api/files/mkdir` - Criar diretório

**Stream:**
- `GET /stream` - Stream MJPEG da câmera

## ⚠️ Importante

### Economia de Memória

Os arquivos HTML/CSS/JS agora são servidos do cartão SD em vez de estarem embutidos no código do ESP32. Isso economiza significativamente a memória flash do microcontrolador.

### Sem Cartão SD

Se o cartão SD não estiver presente:
- O sistema ainda funciona com funcionalidade limitada
- Uma interface básica embutida será exibida
- O File Manager não estará disponível
- Configurações padrão serão usadas

### Solução de Problemas

**SD Card não reconhecido:**
1. Verifique se está formatado em FAT32
2. Tente um cartão diferente
3. Verifique as conexões físicas
4. Consulte o Serial Monitor para mensagens de erro

**Arquivos não carregam:**
1. Verifique se a estrutura de pastas está correta
2. Certifique-se que os arquivos foram copiados corretamente
3. Verifique permissões de leitura dos arquivos

**WiFi não conecta:**
1. Verifique SSID e senha no config.json
2. Tente usar modo AP
3. Verifique o Serial Monitor para mensagens de erro

## 📄 Licença

Este projeto é de código aberto. Consulte o arquivo LICENSE para mais detalhes.
