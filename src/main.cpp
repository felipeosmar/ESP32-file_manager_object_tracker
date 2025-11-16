/**
 * ESP32 Object Tracker with Pan/Tilt Control
 *
 * Hardware:
 * - ESP32-CAM (or ESP32 with OV2640 camera)
 * - 2x Servo motors (Pan and Tilt)
 * - SD Card module
 *
 * Features:
 * - Web interface served from SD card
 * - Real-time camera streaming
 * - Motion detection and object tracking
 * - Automatic pan/tilt adjustment to center objects
 * - Configuration via JSON file on SD card
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SD_MMC.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "camera_config.h"
#include "motion_detector.h"
#include "servo_controller.h"
#include "web_server.h"
#include "sd_manager.h"

// Global objects
AsyncWebServer server(80);
MotionDetector motionDetector;
ServoController servoController;
SDManager sdManager;

// Configuration
struct Config {
  char ssid[32];
  char password[64];
  bool apMode;
  int motionThreshold;
  int trackingSpeed;
  bool autoTracking;
} config;

// Function declarations
bool initCamera();
void setupWiFi();
void setupWebServer();
bool loadConfig();
void setDefaultConfig();
String getBuiltinHTML();
void streamJpg(AsyncWebServerRequest *request);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== ESP32 Object Tracker ===");

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

  // Initialize servos
  Serial.println("Initializing servos...");
  servoController.begin(SERVO_PAN_PIN, SERVO_TILT_PIN);
  servoController.setCenter();
  Serial.println("Servos initialized and centered");

  // Initialize motion detector
  motionDetector.begin(config.motionThreshold);

  // Setup WiFi
  setupWiFi();

  // Setup web server
  setupWebServer();

  Serial.println("\n=== System Ready ===");
  Serial.print("Camera stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.println("====================\n");
}

void loop() {
  static unsigned long lastFrameTime = 0;
  static unsigned long lastTrackingUpdate = 0;
  const unsigned long frameInterval = 100; // 10 FPS for motion detection
  const unsigned long trackingInterval = 50; // 20 Hz for servo updates

  unsigned long currentTime = millis();

  // Process motion detection and tracking
  if (config.autoTracking && (currentTime - lastFrameTime >= frameInterval)) {
    lastFrameTime = currentTime;

    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      // Detect motion and get object center
      Point objectCenter = motionDetector.detectMotion(fb->buf, fb->width, fb->height);

      if (objectCenter.x != -1 && objectCenter.y != -1) {
        // Object detected - calculate error from center
        int centerX = fb->width / 2;
        int centerY = fb->height / 2;
        int errorX = objectCenter.x - centerX;
        int errorY = objectCenter.y - centerY;

        // Update servo positions to center the object
        if (currentTime - lastTrackingUpdate >= trackingInterval) {
          lastTrackingUpdate = currentTime;
          servoController.updateTracking(errorX, errorY, config.trackingSpeed);
        }
      }

      esp_camera_fb_return(fb);
    }
  }

  // Small delay to prevent watchdog issues
  delay(1);
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  // Camera sensor settings
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);
  s->set_quality(s, 12);
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
      WiFi.softAP("ESP32-Tracker", "12345678");
      Serial.print("AP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  }
}

void setupWebServer() {
  Serial.println("Setting up web server...");

  // Serve static files from SD card
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/index.html", "text/html");
    } else {
      request->send(200, "text/html", getBuiltinHTML());
    }
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      Serial.println("Serving style.css");
      AsyncWebServerResponse *response = request->beginResponse(SD_MMC, "/web/style.css", "text/css");
      response->addHeader("Cache-Control", "public, max-age=3600");
      request->send(response);
    } else {
      Serial.println("style.css - SD not ready");
      request->send(404, "text/plain", "SD card not available");
    }
  });

  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      Serial.println("Serving app.js");
      AsyncWebServerResponse *response = request->beginResponse(SD_MMC, "/web/app.js", "application/javascript");
      response->addHeader("Cache-Control", "public, max-age=3600");
      request->send(response);
    } else {
      Serial.println("app.js - SD not ready");
      request->send(404, "text/plain", "SD card not available");
    }
  });

  // Camera stream endpoint - MJPEG streaming
  server.on("/stream", HTTP_GET, streamJpg);

  // API endpoints
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["tracking"] = config.autoTracking;
    doc["pan"] = servoController.getPanAngle();
    doc["tilt"] = servoController.getTiltAngle();
    doc["motion_threshold"] = config.motionThreshold;
    doc["tracking_speed"] = config.trackingSpeed;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/tracking", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("enabled", true)) {
      config.autoTracking = request->getParam("enabled", true)->value() == "true";
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/center", HTTP_POST, [](AsyncWebServerRequest *request) {
    servoController.setCenter();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/manual", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("pan", true)) {
      int pan = request->getParam("pan", true)->value().toInt();
      servoController.setPan(pan);
    }
    if (request->hasParam("tilt", true)) {
      int tilt = request->getParam("tilt", true)->value().toInt();
      servoController.setTilt(tilt);
    }
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Serve CSS and JS files for File Manager
  server.on("/filemanager.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/filemanager.css", "text/css");
    } else {
      request->send(404, "text/plain", "CSS file not available - SD card required");
    }
  });

  server.on("/filemanager.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/filemanager.js", "application/javascript");
    } else {
      request->send(404, "text/plain", "JavaScript file not available - SD card required");
    }
  });

  // File Manager endpoints
  server.on("/filemanager", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (sdManager.isReady()) {
      request->send(SD_MMC, "/web/filemanager.html", "text/html");
    } else {
      request->send(503, "text/html",
        "<html><body><h1>File Manager unavailable</h1>"
        "<p>SD card is required for File Manager functionality.</p>"
        "<a href='/'>Back to Home</a></body></html>");
    }
  });

  // List files in directory
  server.on("/api/files/list", HTTP_GET, [](AsyncWebServerRequest *request) {
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

  // Delete file
  server.on("/api/files/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
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

  strlcpy(config.ssid, doc["wifi"]["ssid"] | "ESP32-Tracker", sizeof(config.ssid));
  strlcpy(config.password, doc["wifi"]["password"] | "12345678", sizeof(config.password));
  config.apMode = doc["wifi"]["ap_mode"] | false;
  config.motionThreshold = doc["tracking"]["motion_threshold"] | 30;
  config.trackingSpeed = doc["tracking"]["speed"] | 5;
  config.autoTracking = doc["tracking"]["auto_enabled"] | true;

  Serial.println("Configuration loaded from SD card");
  return true;
}

void setDefaultConfig() {
  strcpy(config.ssid, "ESP32-Tracker");
  strcpy(config.password, "12345678");
  config.apMode = true;
  config.motionThreshold = 30;
  config.trackingSpeed = 5;
  config.autoTracking = true;
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
        // Frame rate limit
        if (now - lastFrameTime < 100) {  // 100ms = 10 FPS
          delay(100 - (now - lastFrameTime));
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

        frameOffset = 0;
        headerSent = false;
        lastFrameTime = millis();
        frameCount++;

        // Only log every 10th frame to reduce CPU usage
        if (frameCount % 10 == 0) {
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

String getBuiltinHTML() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Object Tracker</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
    h1 { color: #333; }
    img { width: 100%; border-radius: 5px; }
    .controls { margin-top: 20px; }
    button { padding: 10px 20px; margin: 5px; border: none; background: #007bff; color: white; border-radius: 5px; cursor: pointer; }
    button:hover { background: #0056b3; }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Object Tracker</h1>
    <p>SD card not available - using built-in interface</p>
    <img src="/stream" alt="Camera Stream">
    <div class="controls">
      <button onclick="fetch('/api/center', {method: 'POST'})">Center</button>
      <button onclick="toggleTracking()">Toggle Tracking</button>
    </div>
  </div>
  <script>
    function toggleTracking() {
      fetch('/api/tracking', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'enabled=true'
      });
    }
  </script>
</body>
</html>
  )HTML";
}
