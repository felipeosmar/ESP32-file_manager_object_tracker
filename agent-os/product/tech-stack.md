# Tech Stack

## Hardware Platform

### Microcontroller
- **Board:** ESP32-CAM (AI-Thinker module) or compatible ESP32 with OV2640 camera
- **Chip:** ESP32 Dual-Core Xtensa LX6 @ 240MHz
- **Memory:** 520KB SRAM, 4MB Flash (with PSRAM support)
- **Connectivity:** WiFi 802.11 b/g/n (2.4GHz)

### Peripherals
- **Camera:** OV2640 (2MP, up to VGA 640x480 for streaming)
- **Servos:** 2x Standard Hobby Servos (SG90 or similar, 0-180 degrees)
  - Pan servo on GPIO 12
  - Tilt servo on GPIO 13
- **Storage:** MicroSD Card (FAT32 formatted, Class 10 recommended)
- **Power:** 5V @ 2A minimum (higher current recommended for servo operation)

## Backend / Embedded Firmware

### Framework & Runtime
- **Framework:** Arduino Core for ESP32
- **Build System:** PlatformIO (platform: espressif32)
- **Language:** C++ (Arduino-compatible)
- **Board Configuration:** esp32cam target with PSRAM support

### Core Libraries
- **ESPAsyncWebServer** (v1.2.6+) - Asynchronous HTTP/WebSocket server
  - Source: https://github.com/ESP32Async/ESPAsyncWebServer.git
  - Provides non-blocking web server with MJPEG streaming support

- **AsyncTCP** (v1.1.4+) - Asynchronous TCP library for ESPAsyncWebServer
  - Source: https://github.com/dvarrel/AsyncTCP
  - Handles TCP connections asynchronously on ESP32

- **ArduinoJson** (v7.0.4+) - JSON parsing and serialization
  - Efficient JSON handling for configuration files and API responses

- **ESP32Servo** (v1.2.1+) - PWM servo motor control
  - Hardware PWM control for smooth pan/tilt movement

### Native ESP32 Libraries
- **esp_camera** - ESP32 camera driver for OV2640
- **SD_MMC** - SD card access via MMC interface (1-bit mode)
- **WiFi** - ESP32 WiFi stack (Station and Access Point modes)
- **SPIFFS** (optional) - Internal flash filesystem (currently using SD card)

### Future Backend Additions
- **TensorFlow Lite for Microcontrollers** - On-device ML inference
  - Optimized for ESP32 with quantized model support
- **MQTT Client** (PubSubClient or AsyncMQTT) - MQTT protocol implementation
- **ESP32 OTA** - Over-the-air firmware update support
- **NTPClient** - Time synchronization for scheduled recording

## Frontend / Web Interface

### Core Technologies
- **HTML5** - Semantic markup for interface structure
- **CSS3** - Modern styling with Flexbox/Grid layouts, animations
- **Vanilla JavaScript (ES6+)** - No framework dependencies for minimal footprint

### Key Frontend Features
- **MJPEG Streaming** - Native `<img>` tag with multipart/x-mixed-replace
- **Fetch API** - Modern HTTP requests for REST API calls
- **LocalStorage** - Client-side preference persistence
- **Responsive Design** - Mobile-first layout supporting phones, tablets, desktops

### Served From
- **Primary:** SD Card (`/web/` directory)
  - index.html - Main tracker interface
  - style.css - Global styles
  - app.js - Main application logic
  - filemanager.html/css/js - File management interface
  - health.html/css/js - System monitoring dashboard
- **Fallback:** Built-in HTML in firmware (minimal interface when SD unavailable)

## Data Storage & Configuration

### Configuration
- **Format:** JSON (config.json on SD card root)
- **Includes:** WiFi credentials, tracking parameters, servo limits, PID gains
- **Parsing:** ArduinoJson library with dynamic document allocation

### File Storage
- **SD Card Filesystem:** FAT32 (SD_MMC interface)
- **Web Assets:** `/web/` directory (HTML, CSS, JS)
- **User Data:** Root directory for logs, recordings, ML models (future)
- **Thread Safety:** Mutex-protected SD access (`SemaphoreHandle_t`)

### Future Storage
- **Video Recordings:** MJPEG format on SD card
- **ML Models:** TensorFlow Lite (.tflite files) on SD card
- **Event Logs:** JSON-formatted detection events
- **Time-Series Data:** CSV logs for tracking analytics

## Communication Protocols

### HTTP/HTTPS
- **Web Server:** ESPAsyncWebServer (port 80)
- **REST API:** JSON responses for status, control endpoints
- **Streaming:** MJPEG via multipart/x-mixed-replace boundary frames
- **File Upload:** Multipart form data with chunked processing

### WebSocket (Future)
- **Real-time Events:** Detection notifications, tracking updates
- **Bi-directional Control:** Low-latency servo control

### MQTT (Future)
- **Protocol:** MQTT 3.1.1 over TCP
- **Topics:** Configurable base topic for events, status, commands
- **QoS:** Configurable (0, 1, 2) based on message importance
- **Home Assistant:** MQTT Discovery protocol support

## Development Tools & Workflow

### Build System
- **PlatformIO:** Version-controlled dependency management
- **platformio.ini:** Centralized project configuration
- **Build Flags:** Debug levels, PSRAM support, optimization settings

### Development Environment
- **IDE Options:** VSCode with PlatformIO extension (recommended), PlatformIO CLI
- **Serial Monitor:** Built-in PlatformIO monitor with ESP32 exception decoder
- **Upload:** USB serial (FTDI/CP2102) at 921600 baud

### Debugging
- **Serial Logging:** Configurable debug levels (CORE_DEBUG_LEVEL)
- **Web Health Dashboard:** Live system diagnostics (memory, WiFi, CPU)
- **Exception Decoder:** PlatformIO monitor filter for stack trace analysis

### Version Control
- **Git:** Source code and configuration management
- **Branches:** Feature branches for development, main for stable releases

## Compiler & Optimization

### Build Configuration
- **Platform:** Espressif32 (ESP-IDF based)
- **Board:** esp32cam with PSRAM enabled
- **Partition Scheme:** huge_app.csv (for OTA support and large applications)
- **CPU Frequency:** 240MHz
- **Flash Mode:** DIO (Dual I/O)
- **Flash Frequency:** 80MHz

### Optimization Flags
- **PSRAM Support:** `-DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue`
- **Async Configuration:** `-DCONFIG_ASYNC_TCP_RUNNING_CORE=1`
- **Watchdog:** `-DCONFIG_ASYNC_TCP_USE_WDT=0` (disabled for streaming)
- **Web Server:** `-DWEBSERVER_MAX_CONTENT_SIZE=8192` (larger file uploads)
- **Compiler:** `-fpermissive -Wno-deprecated-declarations`

## Testing & Quality (Current State)

### Manual Testing
- **Serial Monitor:** Real-time logging during development
- **Web Interface:** Interactive testing of all features
- **Hardware Testing:** Physical servo movement, camera stream validation

### Future Testing Infrastructure
- **Unit Tests:** PlatformIO native testing framework
- **Integration Tests:** API endpoint validation
- **Performance Tests:** Memory leak detection, frame rate benchmarks
- **Linting:** clang-format for C++ code style consistency

## Deployment

### Initial Deployment
1. **Firmware Upload:** USB serial connection via PlatformIO
2. **SD Card Preparation:** Copy web files and config.json
3. **WiFi Configuration:** Edit config.json with network credentials
4. **Power Cycle:** Insert SD card and power on device

### Future OTA Deployment
- **Web Upload:** Upload .bin file through web interface
- **MQTT Update:** Remote firmware push via MQTT message
- **Rollback:** Automatic fallback on failed update

## Performance Characteristics

### Resource Usage
- **Heap Memory:** ~50-100KB free during operation (monitoring threshold)
- **PSRAM:** Used for camera frame buffers and large allocations
- **CPU Load:** Core 0 for application, Core 1 for WiFi/networking
- **Power Consumption:** ~0.5W nominal, higher during servo movement

### Streaming Performance
- **Frame Rate:** ~10 FPS for MJPEG stream (configurable)
- **Resolution:** VGA (640x480) for display, QVGA (320x240) for processing
- **Latency:** ~200-500ms camera-to-browser depending on network
- **Motion Detection:** 10 FPS analysis, 20Hz servo updates

### Network Requirements
- **Bandwidth:** ~500Kbps for VGA streaming at 10 FPS
- **WiFi Range:** Standard 2.4GHz ESP32 range (50-100m line of sight)
- **Concurrent Clients:** 2-3 simultaneous stream viewers recommended

## Future Technology Integrations

### Machine Learning
- **TensorFlow Lite for Microcontrollers** - Quantized model inference
- **Edge Impulse** - Alternative ML framework for ESP32
- **Model Formats:** .tflite with 8-bit quantization

### Home Automation
- **MQTT Brokers:** Mosquitto, HiveMQ, AWS IoT Core
- **Home Assistant** - Auto-discovery via MQTT
- **OpenHAB** - MQTT binding support

### Cloud Services (Optional)
- **Image Upload:** AWS S3, Google Cloud Storage for recordings
- **Remote Access:** Ngrok, Tailscale, or CloudFlare Tunnels
- **Analytics:** Cloud-based ML training on collected footage
