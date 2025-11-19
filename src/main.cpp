/**
 * ESP32-CAM File Manager with OTA Updates
 *
 * Hardware:
 * - ESP32-CAM
 * - SD Card module
 *
 * Features:
 * - Web interface served from SD card
 * - Real-time camera streaming
 * - File manager (upload, download, edit, delete)
 * - Configuration via JSON file on SD card
 * - Over-the-air (OTA) firmware updates
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SD_MMC.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_task_wdt.h>
#include "esp_camera.h"
#include "camera_config.h"
#include "web_server.h"
#include "sd_manager.h"

// Global objects
AsyncWebServer server(80);
SDManager sdManager;

// Mutex for SD card access (prevents concurrent access issues)
SemaphoreHandle_t sdCardMutex = NULL;

// OTA update flags
bool otaUploadInProgress = false;
bool firstRequestAfterBoot = true;
bool cameraActive = true; // Flag to control camera access during OTA

// Configuration
struct Config {
  char ssid[32];
  char password[64];
  bool apMode;
} config;

// Function declarations
bool initCamera();
void setupWiFi();
void setupWebServer();
bool loadConfig();
void setDefaultConfig();
String getBuiltinHTML();
void streamJpg(AsyncWebServerRequest *request);
void serveStaticFile(AsyncWebServerRequest *request, const char* filepath, const char* contentType);
bool isValidESP32Firmware(uint8_t *data, size_t len);
void validateOTABoot();

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32-CAM File Manager ===");

  // Create mutex for SD card access
  sdCardMutex = xSemaphoreCreateMutex();
  if (sdCardMutex == NULL) {
    Serial.println("Failed to create SD card mutex!");
  }

  // Initialize SD card first
  Serial.println("Initializing SD card...");
  if (!sdManager.begin()) {
    Serial.println("SD Card initialization failed!");
    Serial.println("WARNING: Running without SD card - limited functionality");
  } else {
    Serial.println("SD Card initialized successfully");
  }

  // Load configuration from SD card
  if (!loadConfig()) {
    Serial.println("Failed to load config, using defaults");
    setDefaultConfig();
  }

  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while(1) {
      delay(1000);
      Serial.println("Camera initialization failed - halted");
    }
  }
  Serial.println("Camera initialized successfully");

  // Setup WiFi
  setupWiFi();

  // Setup web server
  setupWebServer();

  Serial.println("\nC=== System Ready ===");
  Serial.print("Camera stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.println("====================\n");
}

void loop() {
  // Skip camera operations if OTA upload is in progress
  if (!cameraActive) {
    delay(100);
    return;
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;  // 20MHz is more stable for OV2640 sensor
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; //Opções: QVGA (320x240), VGA (640x480), SVGA (800x600)
  config.jpeg_quality = 12;  // Higher value = more compression, more stable (10-63 range)
  config.fb_count = 2;  // 2 buffers is more stable than 3 for high FPS

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Camera sensor settings
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_quality(s, 12);  // 12 is a good balance between quality and stability
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);

  return true;
}

void setupWiFi() {
  Serial.println("Setting up WiFi...");

  if (config.apMode) {
    // Access Point mode
    WiFi.softAP(config.ssid, config.password);
    Serial.print("AP Mode - SSID: ");
    Serial.println(config.ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    // Station mode
    WiFi.begin(config.ssid, config.password);
    Serial.print("Connecting to WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect, switching to AP mode");
      WiFi.softAP("ESP32-CAM", "12345678");
      Serial.print("AP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  }
}

/**
 * Validate ESP32 firmware binary format
 * ESP32 binaries start with magic byte 0xE9
 *
 * @param data Pointer to first bytes of file
 * @param len Length of data buffer
 * @return true if valid ESP32 binary, false otherwise
 */
bool isValidESP32Firmware(uint8_t *data, size_t len) {
  if (len < 1) {
    Serial.println("Firmware validation failed: data too short");
    return false;
  }

  // ESP32 binary magic byte
  const uint8_t ESP32_MAGIC_BYTE = 0xE9;

  if (data[0] != ESP32_MAGIC_BYTE) {
    Serial.printf("Invalid firmware: magic byte is 0x%02X, expected 0xE9\n", data[0]);
    return false;
  }

  Serial.println("Firmware validation passed: ESP32 magic byte detected");
  return true;
}

/**
 * Validate OTA boot after firmware update
 * Called on first HTTP request after boot to mark partition as valid
 * Prevents automatic rollback by ESP32 bootloader
 */
void validateOTABoot() {
  if (!firstRequestAfterBoot) {
    return; // Already validated
  }

  firstRequestAfterBoot = false;

  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;

  if (esp_ota_get_state_partition(running, &ota_state) != ESP_OK) {
    Serial.println("Failed to get OTA partition state");
    return;
  }

  if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
    Serial.println("First boot after OTA update detected");
    Serial.println("Web server responding successfully - marking partition valid");

    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      Serial.println("OTA update validated successfully - rollback cancelled");
    } else {
      Serial.println("Failed to mark OTA partition valid");
    }
  } else if (ota_state == ESP_OTA_IMG_VALID) {
    Serial.println("Running from valid OTA partition");
  } else if (ota_state == ESP_OTA_IMG_INVALID) {
    Serial.println("Running from invalid partition (should not happen)");
  }
}

void setupWebServer() {
  Serial.println("Setting up web server...");

  // Serve static files from SD card
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    validateOTABoot();
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/index.html", "text/html");
    } else {
      request->send(200, "text/html", getBuiltinHTML());
    }
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/style.css", "text/css");
  });

  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/app.js", "application/javascript");
  });

  // Camera stream endpoint - MJPEG streaming
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Block stream requests during OTA upload
    if (otaUploadInProgress) {
      request->send(503, "text/plain", "Service unavailable - firmware update in progress");
      return;
    }
    streamJpg(request);
  });

  // Health check endpoint with system diagnostics
  server.on("/api/health/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;

    // System uptime
    unsigned long uptimeMs = millis();
    unsigned long uptimeSec = uptimeMs / 1000;
    unsigned long days = uptimeSec / 86400;
    unsigned long hours = (uptimeSec % 86400) / 3600;
    unsigned long minutes = (uptimeSec % 3600) / 60;
    unsigned long seconds = uptimeSec % 60;

    doc["uptime"]["milliseconds"] = uptimeMs;
    doc["uptime"]["formatted"] = String(days) + "d " + String(hours) + "h " +
                                  String(minutes) + "m " + String(seconds) + "s";

    // Memory information
    doc["memory"]["heap"]["total"] = ESP.getHeapSize();
    doc["memory"]["heap"]["free"] = ESP.getFreeHeap();
    doc["memory"]["heap"]["used"] = ESP.getHeapSize() - ESP.getFreeHeap();
    doc["memory"]["heap"]["usage_percent"] = ((float)(ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize()) * 100;

    doc["memory"]["psram"]["total"] = ESP.getPsramSize();
    doc["memory"]["psram"]["free"] = ESP.getFreePsram();
    doc["memory"]["psram"]["used"] = ESP.getPsramSize() - ESP.getFreePsram();
    if (ESP.getPsramSize() > 0) {
      doc["memory"]["psram"]["usage_percent"] = ((float)(ESP.getPsramSize() - ESP.getFreePsram()) / ESP.getPsramSize()) * 100;
    }

    // WiFi information
    doc["wifi"]["connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["wifi"]["signal_strength"] = WiFi.RSSI() > -50 ? "Excellent" :
                                      WiFi.RSSI() > -60 ? "Good" :
                                      WiFi.RSSI() > -70 ? "Fair" : "Weak";
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["mac"] = WiFi.macAddress();
    doc["wifi"]["channel"] = WiFi.channel();

    // SD Card information
    doc["sd_card"]["ready"] = sdManager.isReady();
    if (sdManager.isReady()) {
      uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
      uint64_t totalBytes = SD_MMC.totalBytes() / (1024 * 1024);
      uint64_t usedBytes = SD_MMC.usedBytes() / (1024 * 1024);
      uint64_t freeBytes = totalBytes - usedBytes;

      doc["sd_card"]["card_size_mb"] = cardSize;
      doc["sd_card"]["total_mb"] = totalBytes;
      doc["sd_card"]["used_mb"] = usedBytes;
      doc["sd_card"]["free_mb"] = freeBytes;
      doc["sd_card"]["usage_percent"] = totalBytes > 0 ? ((float)usedBytes / totalBytes) * 100 : 0;
      doc["sd_card"]["type"] = SD_MMC.cardType() == CARD_MMC ? "MMC" :
                               SD_MMC.cardType() == CARD_SD ? "SDSC" :
                               SD_MMC.cardType() == CARD_SDHC ? "SDHC" : "Unknown";
    }

    // CPU information
    doc["cpu"]["frequency_mhz"] = ESP.getCpuFreqMHz();
    doc["cpu"]["cores"] = 2; // ESP32 has 2 cores
    doc["cpu"]["chip_model"] = ESP.getChipModel();
    doc["cpu"]["chip_revision"] = ESP.getChipRevision();
    doc["cpu"]["sdk_version"] = ESP.getSdkVersion();

    // Flash information
    doc["flash"]["size_mb"] = ESP.getFlashChipSize() / (1024 * 1024);
    doc["flash"]["speed_mhz"] = ESP.getFlashChipSpeed() / 1000000;

    // OTA status
    doc["ota"]["upload_in_progress"] = otaUploadInProgress;

    // Overall health status
    bool isHealthy = WiFi.status() == WL_CONNECTED &&
                     ESP.getFreeHeap() > 50000 && // At least 50KB free heap
                     (!sdManager.isReady() || SD_MMC.totalBytes() > SD_MMC.usedBytes()); // SD not full

    doc["status"] = isHealthy ? "healthy" : "degraded";
    doc["timestamp"] = uptimeMs;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Serve CSS and JS files for File Manager
  server.on("/filemanager.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/filemanager.css", "text/css");
  });

  server.on("/filemanager.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/filemanager.js", "application/javascript");
  });

  // Serve CSS and JS files for Health Monitor
  server.on("/health.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/health.css", "text/css");
  });

  server.on("/health.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/health.js", "application/javascript");
  });

  // Health Monitor page
  server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request) {
    validateOTABoot();
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/health.html", "text/html");
    } else {
      request->send(503, "text/html",
        "<html><body><h1>Health Monitor unavailable</h1>"
        "<p>SD card is required for Health Monitor functionality.</p>"
        "<a href='/'>Back to Home</a></body></html>");
    }
  });

  // File Manager endpoints
  server.on("/filemanager", HTTP_GET, [](AsyncWebServerRequest *request) {
    validateOTABoot();
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/filemanager.html", "text/html");
    } else {
      request->send(503, "text/html",
        "<html><body><h1>File Manager unavailable</h1>"
        "<p>SD card is required for File Manager functionality.</p>"
        "<a href='/'>Back to Home</a></body></html>");
    }
  });

  // Firmware update page and assets
  server.on("/firmware", HTTP_GET, [](AsyncWebServerRequest *request) {
    validateOTABoot();
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/firmware.html", "text/html");
    } else {
      request->send(503, "text/html",
        "<html><body><h1>Firmware Update unavailable</h1>"
        "<p>SD card is required for Firmware Update functionality.</p>"
        "<a href='/'>Back to Home</a></body></html>");
    }
  });

  server.on("/firmware.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/firmware.css", "text/css");
  });

  server.on("/firmware.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveStaticFile(request, "/web/firmware.js", "application/javascript");
  });

  // OTA Firmware Upload endpoint
  // Static variable to track upload errors across callbacks
  static String otaUploadError = "";

  server.on("/api/firmware/upload", HTTP_POST,
    // Response callback (executed after upload completes)
    [](AsyncWebServerRequest *request) {
      // Release SD card mutex
      if (otaUploadInProgress) {
        xSemaphoreGive(sdCardMutex);
        otaUploadInProgress = false;
        Serial.println("OTA upload finished - SD card mutex released");
      }

      // Check for custom error from upload callback
      if (otaUploadError.length() > 0) {
        Serial.printf("OTA Upload error: %s\n", otaUploadError.c_str());
        request->send(500, "application/json",
          "{\"error\":\"" + otaUploadError + "\"}");
        otaUploadError = ""; // Reset error

        // Re-enable camera
        cameraActive = true;
        Serial.println("Camera access resumed after error");

        // Try to reinitialize camera
        delay(100);
        if (initCamera()) {
          Serial.println("Camera reinitialized successfully after OTA error");
        }
        return;
      }

      // Check for Update library errors
      if (Update.hasError()) {
        String error = "Update failed. Error: ";
        error += Update.errorString();
        Serial.println(error);
        request->send(500, "application/json",
          "{\"error\":\"" + error + "\"}");

        // Re-enable camera
        cameraActive = true;
        Serial.println("Camera access resumed after error");

        // Try to reinitialize camera
        delay(100);
        if (initCamera()) {
          Serial.println("Camera reinitialized successfully after OTA error");
        }
        return;
      }

      // Success - send response and reboot
      Serial.println("OTA Update successful! Rebooting...");
      request->send(200, "application/json",
        "{\"status\":\"ok\",\"message\":\"Firmware updated successfully. Device will reboot now.\"}");

      // Give enough time for response to be fully transmitted to client
      delay(2000);
      Serial.println("Restarting ESP32 now...");
      ESP.restart();
    },

    // Upload chunk callback (executed for each data chunk)
    [](AsyncWebServerRequest *request, String filename, size_t index,
       uint8_t *data, size_t len, bool final) {

      // First chunk - initialize OTA update
      if (index == 0) {
        Serial.printf("\n=== OTA Update started: %s ===\n", filename.c_str());
        Serial.printf("File size: %d bytes\n", request->contentLength());
        otaUploadError = ""; // Reset error flag

        // Disable watchdog for this task to prevent timeout during camera deinit
        Serial.println("[0/6] Disabling watchdog timer...");
        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
        esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));

        // Stop camera access from loop() task
        cameraActive = false;
        Serial.println("[1/6] Camera access paused");
        delay(300); // Increased delay to ensure camera operations stop

        // Deinitialize camera to free shared pins (SD card shares pins with camera)
        Serial.println("[2/6] Deinitializing camera...");
        esp_err_t err = esp_camera_deinit();
        if (err != ESP_OK) {
          Serial.printf("Camera deinit warning: 0x%x\n", err);
        } else {
          Serial.println("Camera deinitialized successfully");
        }
        delay(200); // Increased delay for camera to fully stop

        // Free up memory before OTA
        Serial.println("[3/6] Freeing memory...");
        Serial.printf("Free heap before OTA: %d bytes\n", ESP.getFreeHeap());

        // Acquire SD card mutex to block file operations
        Serial.println("[4/6] Acquiring SD card mutex...");
        if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(10000)) != pdTRUE) {
          Serial.println("ERROR: SD card busy - mutex timeout");
          otaUploadError = "SD card is busy";
          cameraActive = true; // Re-enable camera on error
          return;
        }
        otaUploadInProgress = true;
        Serial.println("SD card mutex acquired");

        // Validate ESP32 firmware format
        Serial.println("[5/6] Validating firmware...");
        if (!isValidESP32Firmware(data, len)) {
          Serial.println("ERROR: Invalid firmware file - magic byte check failed");
          otaUploadError = "Invalid ESP32 firmware file (magic byte check failed)";
          xSemaphoreGive(sdCardMutex);
          otaUploadInProgress = false;
          cameraActive = true; // Re-enable camera on error
          return;
        }
        Serial.println("Firmware validation passed");

        // Begin OTA update
        Serial.println("[6/6] Initializing OTA update...");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          Serial.printf("ERROR: Update.begin() failed: %s\n", Update.errorString());
          otaUploadError = "Failed to begin OTA update: ";
          otaUploadError += Update.errorString();
          xSemaphoreGive(sdCardMutex);
          otaUploadInProgress = false;
          cameraActive = true; // Re-enable camera on error
          return;
        }

        Serial.println("=== OTA Update initialized - ready to receive data ===\n");
      }

      // Write chunk to flash
      if (len) {
        // Feed watchdog before write operation
        yield();

        size_t written = Update.write(data, len);
        if (written != len) {
          Serial.printf("ERROR: OTA Write failed - wrote %d of %d bytes\n", written, len);
          otaUploadError = "Failed to write firmware data to flash";
          Update.abort();
          return;
        }

        // Feed watchdog after write operation
        yield();

        // Log progress more frequently for debugging
        if (index % 32768 == 0 && index > 0) { // Every 32KB
          Serial.printf("Progress: %d KB written (%.1f%%)\n",
                       (index + len) / 1024,
                       ((float)(index + len) / request->contentLength()) * 100);
          Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        }
      }

      // Final chunk - complete OTA update
      if (final) {
        Serial.println("\n=== Finalizing OTA update ===");
        Serial.printf("Total received: %d bytes\n", index + len);

        if (Update.end(true)) {
          Serial.println("SUCCESS: OTA Update completed!");
          Serial.printf("Final size: %d bytes\n", index + len);
          Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
          Serial.println("Device will reboot after sending response...");
        } else {
          Serial.printf("ERROR: Update.end() failed: %s\n", Update.errorString());
          otaUploadError = "Failed to finalize OTA update: ";
          otaUploadError += Update.errorString();
        }
      }
    }
  );

  // List files in directory
  server.on("/api/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "application/json", "{\"error\":\"System busy - firmware update in progress\"}");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "application/json", "{\"error\":\"SD card not ready\"}");
      return;
    }

    String path = "/";
    if (request->hasParam("dir")) {
      path = request->getParam("dir")->value();
    }

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
      request->send(404, "application/json", "{\"error\":\"Directory not found\"}");
      return;
    }

    JsonDocument doc;
    JsonArray files = doc["files"].to<JsonArray>();

    File file = root.openNextFile();
    while (file) {
      JsonObject fileObj = files.add<JsonObject>();
      fileObj["name"] = String(file.name());
      fileObj["size"] = file.size();
      fileObj["isDir"] = file.isDirectory();
      file = root.openNextFile();
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Download file
  server.on("/api/files/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "text/plain", "System busy - firmware update in progress");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "text/plain", "SD card not ready");
      return;
    }

    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filepath = request->getParam("file")->value();
    if (!SD_MMC.exists(filepath)) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    request->send(SD_MMC, filepath, String(), true);
  });

  // View file content
  server.on("/api/files/view", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "text/plain", "System busy - firmware update in progress");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "text/plain", "SD card not ready");
      return;
    }

    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filepath = request->getParam("file")->value();
    if (!SD_MMC.exists(filepath)) {
      request->send(404, "text/plain", "File not found");
      return;
    }

    request->send(SD_MMC, filepath, "text/plain", false);
  });

  // Read file content for editing (with size limit)
  server.on("/api/files/read", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "application/json", "{\"error\":\"System busy - firmware update in progress\"}");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "application/json", "{\"error\":\"SD card not ready\"}");
      return;
    }

    if (!request->hasParam("file")) {
      request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
      return;
    }

    String filepath = request->getParam("file")->value();
    if (!SD_MMC.exists(filepath)) {
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }

    // Acquire mutex for SD card access
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
      File file = SD_MMC.open(filepath, FILE_READ);
      if (!file) {
        xSemaphoreGive(sdCardMutex);
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
      }

      size_t fileSize = file.size();

      // Limit file size to 50KB for safety
      if (fileSize > 51200) {
        file.close();
        xSemaphoreGive(sdCardMutex);
        request->send(413, "application/json", "{\"error\":\"File too large (max 50KB)\"}");
        return;
      }

      String content = "";
      content.reserve(fileSize + 1);

      while (file.available()) {
        content += (char)file.read();
      }

      file.close();
      xSemaphoreGive(sdCardMutex);

      JsonDocument doc;
      doc["status"] = "ok";
      doc["content"] = content;
      doc["size"] = fileSize;

      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
    } else {
      request->send(503, "application/json", "{\"error\":\"SD card busy\"}");
    }
  });

  // Write file content (save edited file)
  server.on("/api/files/write", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "application/json", "{\"error\":\"System busy - firmware update in progress\"}");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "application/json", "{\"error\":\"SD card not ready\"}");
      return;
    }

    if (!request->hasParam("file", true) || !request->hasParam("content", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing file or content parameter\"}");
      return;
    }

    String filepath = request->getParam("file", true)->value();
    String content = request->getParam("content", true)->value();

    // Acquire mutex for SD card access
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
      File file = SD_MMC.open(filepath, FILE_WRITE);
      if (!file) {
        xSemaphoreGive(sdCardMutex);
        request->send(500, "application/json", "{\"error\":\"Failed to open file for writing\"}");
        return;
      }

      size_t written = file.print(content);
      file.close();
      xSemaphoreGive(sdCardMutex);

      if (written > 0) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["written"] = written;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
      } else {
        request->send(500, "application/json", "{\"error\":\"Failed to write file\"}");
      }
    } else {
      request->send(503, "application/json", "{\"error\":\"SD card busy\"}");
    }
  });

  // Delete file
  server.on("/api/files/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "application/json", "{\"error\":\"System busy - firmware update in progress\"}");
      return;
    }

    if (!sdManager.isReady()) {
      request->send(503, "application/json", "{\"error\":\"SD card not ready\"}");
      return;
    }

    if (!request->hasParam("file", true)) {
      request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
      return;
    }

    String filepath = request->getParam("file", true)->value();

    File file = SD_MMC.open(filepath);
    if (!file) {
      request->send(404, "application/json", "{\"error\":\"File not found\"}");
      return;
    }

    bool isDir = file.isDirectory();
    file.close();

    bool success = false;
    if (isDir) {
      success = SD_MMC.rmdir(filepath);
    } else {
      success = SD_MMC.remove(filepath);
    }

    if (success) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      request->send(500, "application/json", "{\"error\":\"Failed to delete\"}");
    }
  });

  // Upload file
  server.on("/api/files/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      static File uploadFile;

      if (otaUploadInProgress) {
        Serial.println("File upload blocked: OTA in progress");
        return;
      }

      if (!sdManager.isReady()) {
        Serial.println("Upload failed: SD not ready");
        return;
      }

      if (index == 0) {
        // Get directory from query string parameter (GET), not POST body
        String path = "/";
        if (request->hasParam("dir", false)) {  // false = GET parameter
          path = request->getParam("dir", false)->value();
          Serial.printf("Upload - received dir parameter from query string: '%s'\n", path.c_str());

          // Normalize path: ensure it ends with / unless it's just "/"
          if (path != "/" && !path.endsWith("/")) {
            path += "/";
          }
        } else {
          Serial.println("Upload - no dir parameter, using root");
        }

        String filepath = path + filename;
        Serial.printf("Upload start: %s (dir='%s', file='%s')\n",
                      filepath.c_str(), path.c_str(), filename.c_str());

        // Delete existing file to prevent appending to old content
        // FILE_WRITE mode appends if file exists, so we need to remove it first
        if (SD_MMC.exists(filepath)) {
          SD_MMC.remove(filepath);
          Serial.printf("Existing file removed for overwrite: %s\n", filepath.c_str());
        }

        uploadFile = SD_MMC.open(filepath, FILE_WRITE);
        if (!uploadFile) {
          Serial.printf("Failed to open file for writing: %s\n", filepath.c_str());
          return;
        }
      }

      // Write data chunk
      if (uploadFile && len) {
        size_t written = uploadFile.write(data, len);
        if (written != len) {
          Serial.printf("Warning: Only wrote %d of %d bytes\n", written, len);
        }

        // Feed watchdog periodically to prevent timeout on large uploads
        if (index % 8192 == 0) {  // Every ~8KB
          delay(1);  // Small yield to prevent watchdog timeout
        }
      }

      if (final) {
        if (uploadFile) {
          uploadFile.close();
          Serial.printf("Upload complete: %s (%d bytes total)\n", filename.c_str(), index + len);
        }
      }
    }
  );

  // Create directory
  server.on("/api/files/mkdir", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (otaUploadInProgress) {
      request->send(503, "application/json", "{\"error\":\"System busy - firmware update in progress\"}");
      return;
    }

    if (!sdManager.isReady()) {
      Serial.println("Mkdir failed: SD not ready");
      request->send(503, "application/json", "{\"error\":\"SD card not ready\"}");
      return;
    }

    if (!request->hasParam("dir", true)) {
      Serial.println("Mkdir failed: Missing dir parameter");
      request->send(400, "application/json", "{\"error\":\"Missing dir parameter\"}");
      return;
    }

    String dirpath = request->getParam("dir", true)->value();
    Serial.printf("Mkdir - creating directory: '%s'\n", dirpath.c_str());

    if (SD_MMC.mkdir(dirpath)) {
      Serial.printf("Mkdir - success: '%s'\n", dirpath.c_str());
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      Serial.printf("Mkdir - failed: '%s'\n", dirpath.c_str());
      request->send(500, "application/json", "{\"error\":\"Failed to create directory\"}");
    }
  });

  // 404 handler with OTA boot validation
  server.onNotFound([](AsyncWebServerRequest *request) {
    validateOTABoot();
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started");
}

bool loadConfig() {
  if (!sdManager.isReady()) return false;

  File file = SD_MMC.open("/config.json", FILE_READ);
  if (!file) {
    Serial.println("Config file not found");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  strlcpy(config.ssid, doc["wifi"]["ssid"] | "ESP32-CAM", sizeof(config.ssid));
  strlcpy(config.password, doc["wifi"]["password"] | "12345678", sizeof(config.password));
  config.apMode = doc["wifi"]["ap_mode"] | false;

  Serial.println("Configuration loaded from SD card");
  return true;
}

void setDefaultConfig() {
  strcpy(config.ssid, "ESP32-CAM");
  strcpy(config.password, "12345678");
  config.apMode = true;
}

void streamJpg(AsyncWebServerRequest *request) {
  Serial.println("Stream requested");

  AsyncWebServerResponse *response = request->beginChunkedResponse(
    "multipart/x-mixed-replace; boundary=frame",
    [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      static unsigned long lastFrameTime = 0;
      static camera_fb_t *currentFrame = NULL;
      static size_t frameOffset = 0;
      static bool headerSent = false;
      static uint32_t frameCount = 0;

      // Control frame rate (~10 FPS for stability)
      unsigned long now = millis();

      // If we don't have a current frame, get a new one
      if (currentFrame == NULL) {
        // Check if camera is active (not during OTA)
        if (!cameraActive) {
          delay(100);
          return 0; // Stop streaming during OTA
        }

        // Frame rate limit (~16 FPS for stability)
        if (now - lastFrameTime < 60) {  // 60ms = ~16 FPS (more stable than 50ms)
          delay(60 - (now - lastFrameTime));
        }

        currentFrame = esp_camera_fb_get();
        if (!currentFrame) {
          // Only log every 10th failure to reduce serial spam
          static uint8_t failCount = 0;
          if (++failCount >= 10) {
            Serial.println("Camera capture failed");
            failCount = 0;
          }
          delay(100);
          return 0;
        }

        // Validate frame buffer integrity
        if (currentFrame->len == 0 || currentFrame->buf == NULL) {
          Serial.println("Invalid frame buffer detected - skipping");
          esp_camera_fb_return(currentFrame);
          currentFrame = NULL;
          delay(50);
          return 0;
        }

        frameOffset = 0;
        headerSent = false;
        lastFrameTime = millis();
        frameCount++;

        // Only log every 1000th frame to reduce CPU usage
        if (frameCount % 1000 == 0) {
          Serial.printf("Frame #%d: %d bytes\n", frameCount, currentFrame->len);
        }
      }

      size_t written = 0;

      // Send boundary and headers at frame start
      if (!headerSent) {
        String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ";
        header += String(currentFrame->len);
        header += "\r\n\r\n";

        size_t headerLen = header.length();
        if (headerLen > maxLen) {
          esp_camera_fb_return(currentFrame);
          currentFrame = NULL;
          return 0;
        }

        memcpy(buffer, header.c_str(), headerLen);
        written = headerLen;
        headerSent = true;
      }

      // Send image data in chunks
      size_t remainingData = currentFrame->len - frameOffset;
      size_t availableSpace = maxLen - written;
      size_t toSend = (remainingData < availableSpace) ? remainingData : availableSpace;

      if (toSend > 0) {
        memcpy(buffer + written, currentFrame->buf + frameOffset, toSend);
        written += toSend;
        frameOffset += toSend;
      }

      // If frame complete, add CRLF and release
      if (frameOffset >= currentFrame->len) {
        if (written + 2 <= maxLen) {
          buffer[written++] = '\r';
          buffer[written++] = '\n';
        }

        esp_camera_fb_return(currentFrame);
        currentFrame = NULL;
        frameOffset = 0;
        headerSent = false;
      }

      // Yield every 10 chunks
      if (index % 10 == 0) {
        delay(1);
      }

      return written;
    }
  );

  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");

  request->send(response);
  Serial.println("Stream started");
}

// getFileManagerHTML() removed - now served from SD card files to save memory

/**
 * Serve static files from SD card using AsyncFileResponse
 * ESPAsyncWebServer handles async file reading internally, no mutex needed
 */
void serveStaticFile(AsyncWebServerRequest *request, const char* filepath, const char* contentType) {
  if (!sdManager.isReady()) {
    Serial.printf("Cannot serve %s - SD not ready\n", filepath);
    request->send(503, "text/plain", "SD card not available");
    return;
  }

  if (!SD_MMC.exists(filepath)) {
    Serial.printf("File not found: %s\n", filepath);
    request->send(404, "text/plain", "File not found");
    return;
  }

  Serial.printf("Serving %s\n", filepath);

  // AsyncFileResponse handles file reading asynchronously and internally
  // No mutex needed here as ESPAsyncWebServer manages the file access safely
  AsyncWebServerResponse *response = request->beginResponse(SD_MMC, filepath, contentType);

  if (response) {
    response->addHeader("Cache-Control", "public, max-age=3600");
    request->send(response);
  } else {
    Serial.printf("Failed to create response for %s\n", filepath);
    request->send(500, "text/plain", "Failed to serve file");
  }
}

String getBuiltinHTML() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Stream</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
    h1 { color: #333; }
    img { width: 100%; border-radius: 5px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>📷 ESP32-CAM Stream</h1>
    <p>SD card not available - using built-in interface</p>
    <img src="/stream" alt="Camera Stream">
  </div>
</body>
</html>
  )HTML";
}
