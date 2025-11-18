# Specification: Over-the-Air (OTA) Firmware Update

## Overview

### Goal
Enable remote firmware updates for ESP32-CAM devices through a web-based interface, allowing IoT developers to deploy new firmware versions without physical access to the device or USB cable connection.

### User Value
- **Rapid Iteration**: Deploy firmware updates remotely during prototyping and development cycles without physical device access
- **Field Updates**: Update deployed devices in remote locations (security cameras, monitoring systems) without site visits
- **Reduced Downtime**: Quick firmware updates minimize device offline time during maintenance windows
- **Safe Recovery**: Automatic rollback protection ensures devices remain functional even if an update fails

### Success Criteria
- Users can upload and flash firmware .bin files via web interface with real-time progress feedback
- Upload and flashing process completes successfully for typical firmware sizes (1-2MB) within 60-90 seconds
- Device automatically reboots after successful update and serves the web interface within 10 seconds
- Failed updates automatically rollback to previous working firmware without manual intervention
- SD card file operations are blocked during OTA process while system monitoring endpoints remain accessible

## User Stories

### Primary User Flow
- As an IoT developer, I want to upload new firmware via the web interface so that I can update my device without connecting a USB cable
- As a maker deploying devices in hard-to-reach locations, I want firmware updates to work reliably over WiFi so that I don't need to physically retrieve the device for updates
- As a developer testing new features, I want automatic rollback on failed updates so that a bad firmware doesn't permanently break my device

### Edge Cases & Error Scenarios
- As a user, I want clear error messages if my firmware file is invalid so that I understand what went wrong before spending time uploading
- As a developer, I want the upload to be blocked if another operation is in progress so that concurrent operations don't corrupt the firmware
- As a user, I want visible progress indication during long uploads so that I know the system hasn't frozen

## Core Requirements

### Functional Requirements

#### OTA Implementation
- Use ESP32's native `Update` library for firmware flashing operations
- Support HTTP-based firmware upload through ESPAsyncWebServer (reusing existing upload handler pattern)
- Write firmware directly to OTA partition without intermediate SD card storage
- Implement two-partition OTA scheme (factory + ota_0 + ota_1) using ESP32's built-in partition scheme
- Automatic device reboot after successful firmware write completion
- Validate ESP32 binary format by checking magic byte (0xE9) at start of file

#### Web Interface
- Create dedicated `/firmware` page accessible via new header navigation button
- Reuse file manager's drag-and-drop upload UI design for consistency
- Display real-time upload progress bar (percentage and status text)
- Show firmware update workflow stages: uploading → validating → flashing → rebooting
- Prevent page navigation during active firmware update operations
- Display clear error messages for failed validations or upload failures

#### Upload Workflow
1. User selects .bin firmware file via file input or drag-and-drop zone
2. Client-side validation checks file extension is .bin
3. Upload begins with XHR request showing progress bar
4. Server validates ESP32 magic byte (0xE9) at start of uploaded data
5. Firmware streams directly to OTA partition via `Update.write()` calls
6. On completion, device reboots automatically within 2-3 seconds

#### Validation & Security
- Check ESP32 magic byte (0xE9) at start of firmware binary before accepting upload
- Reject non-.bin files with user-friendly error message
- Reject files that don't contain valid ESP32 binary format
- Block concurrent firmware uploads (only one at a time)
- No authentication required initially (trusted network assumption)

#### Error Handling & Safe Mode
- Implement "safe mode" boot mechanism after OTA update completes
- Device enters validation mode after reboot from OTA update
- Success criteria: First successful HTTP request to any endpoint marks boot as valid
- System calls `esp_ota_mark_app_valid_cancel_rollback()` on first HTTP request after OTA
- If web server fails to start or no HTTP request received, automatic rollback occurs
- Display user-facing error messages for upload failures, validation failures, or write errors
- Provide abort mechanism if upload fails mid-process (return buffer, clear update state)

#### Operational Constraints
- Block SD card file operations during firmware upload (using mutex pattern)
- Allow health monitoring endpoint (`/api/health/status`) to continue responding during upload
- Allow other non-file endpoints (status checks, camera stream) to remain accessible
- Feed watchdog timer during long write operations to prevent timeout
- Use mutex to coordinate between OTA upload and SD card operations

### Non-Functional Requirements

#### Performance
- Support firmware files up to 2MB (typical ESP32 application size)
- Stream firmware data without buffering entire file in memory (memory-constrained device)
- Upload speed limited by WiFi bandwidth (~100-200 KB/s typical)
- Flashing process takes approximately 30-60 seconds for 1.5MB firmware
- Total update time (upload + flash + reboot) under 2 minutes for typical firmware

#### Reliability
- Zero data corruption during streaming write to flash partition
- Atomic partition switching (old partition remains bootable until new one validated)
- Rollback occurs automatically if device fails to boot after update
- Safe mode validation window: 30 seconds after reboot to mark partition valid

#### Compatibility
- Works with all ESP32-CAM hardware variants (AI-Thinker and compatible modules)
- Compatible with PlatformIO build system and espressif32 platform
- Supports firmware built with Arduino framework for ESP32
- Uses ESP32's built-in OTA partition schemes (min_spiffs.csv or default_8MB.csv)

## Visual Design

### UI/UX Specifications

#### Page Layout
- Header: Consistent navigation with existing pages (Home, Health, File Manager buttons)
- Title: "Atualização de Firmware" with rocket emoji (🚀)
- Upload zone: Reuse file manager's drag-and-drop design with .bin file filter
- Progress section: Large progress bar with percentage and status text
- Status display: Current firmware info (version, build date - if available in future)

#### Upload Zone Styling
- Dashed border container (similar to file manager upload zone)
- Icon: Firmware chip or microchip emoji
- Text: "Arraste o arquivo .bin aqui ou clique para selecionar"
- Accept attribute: `.bin` file extension only
- Hover state: Border color change on drag-over
- Active state: Different background color while dragging file over zone

#### Progress Feedback
- Progress bar: 0-100% visual fill with animated transition
- Status text stages:
  - "Enviando firmware..." (0-95% during upload)
  - "Validando arquivo..." (95-96% after upload completes)
  - "Gravando na flash..." (96-99% during Update.write operations)
  - "Reiniciando dispositivo..." (100% before reboot)
  - "Atualização completa! Aguarde reconexão..." (after reboot initiated)

#### Error States
- Red error banner for validation failures: "Arquivo inválido: não é um firmware ESP32"
- Orange warning banner for upload failures: "Erro no upload. Tente novamente."
- Red error banner for flash failures: "Erro ao gravar firmware. Dispositivo permanece na versão anterior."

#### Navigation Button
- Location: Header, positioned between "Saúde" and "Arquivos" buttons
- Style: Consistent with existing navigation buttons
- Text: "🚀 Firmware" or "⚙️ Atualização"
- Mobile responsive: Icon-only on small screens

### Responsive Design
- Desktop (>768px): Full layout with side-by-side upload zone and progress display
- Tablet (480-768px): Stacked layout with upload zone above progress bar
- Mobile (<480px): Compact layout with smaller upload zone and full-width progress bar

## Reusable Components

### Existing Code to Leverage

#### Backend Components

**File Upload Handler Pattern** (`/src/main.cpp` lines 644-709)
- ESPAsyncWebServer upload handler structure with three callbacks:
  - Request completion callback (sends final response)
  - Upload chunk callback (processes data chunks)
  - Index tracking for first chunk, intermediate chunks, and final chunk
- Chunk-by-chunk processing pattern reduces memory requirements
- Watchdog feeding every 8KB to prevent timeout on large files
- Query parameter parsing for upload destination (GET parameter, not POST body)
- Pattern: `server.on("/api/endpoint", HTTP_POST, [response], [upload])`

**Adaptation for OTA:**
- Replace `SD_MMC.open()` and `file.write()` with `Update.begin()` and `Update.write()`
- On first chunk (index == 0): Call `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)`
- On each chunk: Call `Update.write(data, len)` instead of file write
- On final chunk: Call `Update.end(true)` and check `Update.hasError()`
- Error handling: Call `Update.abort()` if write fails, return error response

**SD Card Mutex Protection** (`/src/main.cpp` lines 36, 520-560)
- Global `SemaphoreHandle_t sdCardMutex` for thread-safe SD operations
- Pattern: `xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000))` before SD access
- Always release with `xSemaphoreGive(sdCardMutex)` in success and error paths
- Timeout handling: Return 503 "SD card busy" if mutex unavailable

**Adaptation for OTA:**
- Acquire `sdCardMutex` at start of OTA upload to block file operations
- Hold mutex for entire upload duration (prevents concurrent SD card access)
- Release mutex in upload completion callback (success or failure)
- Other endpoints check mutex availability and return "busy" if OTA in progress

#### Frontend Components

**Upload UI Pattern** (`/data/web/filemanager.html` + `/data/web/filemanager.js`)
- Drag-and-drop zone with `dragover`, `dragleave`, `drop` event handlers
- File input element (`<input type="file">`) triggered by zone click
- Visual feedback: Add/remove `dragging` CSS class during drag operations
- File selection handlers for both click and drag-drop

**Progress Bar Implementation** (`/data/web/filemanager.js` lines 198-243)
- XMLHttpRequest (XHR) for upload with progress event tracking
- Progress bar element with animated width transition (0-100%)
- Percentage text overlay on progress fill
- Progress visibility: Hidden by default, shown during upload, hidden after completion
- Pattern:
  ```javascript
  xhr.upload.addEventListener('progress', (e) => {
    const percent = Math.round((e.loaded / e.total) * 100);
    progressFill.style.width = percent + '%';
    progressFill.textContent = percent + '%';
  });
  ```

**Page Structure** (`/data/web/index.html`, `/data/web/health.html`)
- Consistent header with navigation buttons
- Pattern: `<a href="/page" class="btn-name">Icon Label</a>`
- Container structure: `<div class="container">` with `<header>`, `<main>`, `<footer>`
- Responsive CSS grid layouts for sections

### New Components Required

#### Backend: OTA Upload Endpoint Handler
**Why new:** File upload handler writes to SD card; OTA handler writes to flash partition using different API (`Update` library instead of `SD_MMC`)

**Key differences:**
- Initialization: `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)` instead of `SD_MMC.open()`
- Write operation: `Update.write(data, len)` instead of `file.write(data, len)`
- Validation: Check ESP32 magic byte (0xE9) on first chunk before calling `Update.begin()`
- Completion: `Update.end(true)` and check `Update.hasError()` instead of `file.close()`
- Error handling: `Update.abort()` on failure to clean up state

#### Backend: Safe Mode Boot Check
**Why new:** No existing boot validation or rollback mechanism in codebase

**Implementation:**
- Global flag `bool otaUpdatePending` set on successful OTA completion
- On first HTTP request after boot, check if `otaUpdatePending == true`
- If true, call `esp_ota_mark_app_valid_cancel_rollback()` to mark partition valid
- Reset `otaUpdatePending` to false after marking valid
- Store flag in RTC memory or use `esp_ota_get_boot_partition()` to detect OTA boot

#### Backend: Binary Format Validation Function
**Why new:** No existing binary validation logic for firmware files

**Implementation:**
- Function: `bool isValidESP32Binary(uint8_t *data, size_t len)`
- Check: `len >= 1 && data[0] == 0xE9`
- Called on first chunk (index == 0) before `Update.begin()`
- Return error response if validation fails: "Invalid ESP32 firmware file"

#### Frontend: Firmware Upload Page (firmware.html)
**Why new:** Dedicated page for firmware updates doesn't exist yet

**Structure:**
- Header with navigation (reuses existing header pattern)
- Upload zone for .bin files (reuses file manager drag-drop UI)
- Progress bar section (reuses file manager progress bar)
- Status text display (new - shows current update stage)
- Warning section explaining OTA process and risks

#### Frontend: Upload JavaScript Logic (firmware.js)
**Why new:** Firmware upload has different validation and flow than file upload

**Differences from file upload:**
- File type restriction: Only accept .bin files
- Single file upload: Block multiple file selection
- Status stages: Show detailed progress stages (uploading, validating, flashing, rebooting)
- Post-upload behavior: Display "reconnecting" message instead of refresh
- Auto-reconnect: Poll server after reboot to detect when device is back online
- Error handling: Different error messages for firmware-specific failures

## Technical Approach

### Architecture Overview

```
[Browser] --HTTP POST--> [ESPAsyncWebServer] --chunks--> [OTA Upload Handler]
                                                               |
                                                               v
                                                    [ESP32 Update Library]
                                                               |
                                                               v
                                                    [Flash Partition: ota_0/ota_1]
                                                               |
                                                               v
                                                    [Automatic Reboot]
                                                               |
                                                               v
                                                    [Boot from new partition]
                                                               |
                                                               v
                                        [Safe mode: wait for HTTP request]
                                                    /                  \
                                            HTTP received          No HTTP (timeout)
                                                 |                        |
                                                 v                        v
                                    esp_ota_mark_app_valid()    [Rollback to previous]
```

### Component Design

#### Backend Components

**1. OTA Upload Endpoint** (`/api/firmware/upload`)
- Method: HTTP POST
- Content-Type: `multipart/form-data`
- Handler structure:
  ```cpp
  server.on("/api/firmware/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // Response callback (sent after upload completes)
      if (Update.hasError()) {
        request->send(500, "application/json", "{\"error\":\"Firmware update failed\"}");
      } else {
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Rebooting...\"}");
        delay(1000);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // Upload chunk callback
      // Implementation details in "Implementation Details" section
    }
  );
  ```

**2. Safe Mode Validation**
- Location: Early in `setupWebServer()` function, before defining other endpoints
- Trigger: Check on every HTTP request using `server.onNotFound()` or first request flag
- Implementation:
  ```cpp
  bool firstRequestAfterBoot = true;

  // In setupWebServer(), before other endpoints:
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (firstRequestAfterBoot) {
      const esp_partition_t *running = esp_ota_get_running_partition();
      esp_ota_img_states_t ota_state;
      if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
          esp_ota_mark_app_valid_cancel_rollback();
          Serial.println("OTA update validated successfully");
        }
      }
      firstRequestAfterBoot = false;
    }
    // Continue with normal 404 handling
    request->send(404, "text/plain", "Not found");
  });
  ```

**3. SD Card Mutex Integration**
- Acquire mutex at start of OTA upload (first chunk)
- Hold mutex for entire upload duration
- Release mutex in response callback (after upload completes or fails)
- Pattern:
  ```cpp
  static bool otaMutexAcquired = false;

  if (index == 0) {
    if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
      otaMutexAcquired = true;
      // Proceed with Update.begin()
    } else {
      // Return error: system busy
      return;
    }
  }

  // In response callback:
  if (otaMutexAcquired) {
    xSemaphoreGive(sdCardMutex);
    otaMutexAcquired = false;
  }
  ```

**4. Firmware Page Endpoint** (`/firmware`)
- Method: HTTP GET
- Response: Serve `/web/firmware.html` from SD card
- Fallback: Return error message if SD card unavailable
- Pattern identical to `/filemanager` and `/health` endpoints

**5. Static Asset Endpoints**
- `/firmware.css` - Stylesheet for firmware page
- `/firmware.js` - JavaScript for firmware upload logic
- Pattern identical to existing static file serving

#### Frontend Components

**1. Firmware Upload Page** (`firmware.html`)
- Header with navigation (Home, Health, Files, Firmware)
- Upload zone with drag-and-drop support
- Progress bar with percentage and status text
- Warning/info section about OTA process
- Responsive layout (mobile-first design)

**2. Upload JavaScript** (`firmware.js`)
- File selection handling (click and drag-drop)
- Client-side validation (file extension, size check)
- XHR upload with progress tracking
- Status text updates for each stage
- Error handling and user feedback
- Auto-reconnect polling after reboot

**3. CSS Styling** (`firmware.css`)
- Reuse file manager styles for consistency
- Upload zone styling with firmware-specific icons
- Progress bar animations
- Warning banner styles
- Responsive breakpoints

### Data Flow

#### Upload Process
1. **Client Selection**: User selects .bin file via file input or drag-drop
2. **Client Validation**: JavaScript checks file extension is `.bin`
3. **Upload Initiation**: XHR POST request to `/api/firmware/upload`
4. **Server Validation**: First chunk arrives, server checks magic byte (0xE9)
5. **Update Begin**: Server calls `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)`
6. **Chunk Processing**: Server calls `Update.write(data, len)` for each chunk
7. **Watchdog Feeding**: Every 8KB, server calls `delay(1)` to feed watchdog
8. **Upload Complete**: Final chunk arrives, server calls `Update.end(true)`
9. **Success Response**: Server sends 200 OK response with reboot message
10. **Reboot**: Server waits 1 second, then calls `ESP.restart()`

#### Boot & Validation Process
1. **Device Boots**: ESP32 boots from newly flashed OTA partition
2. **Application Start**: `setup()` function executes
3. **Web Server Start**: `setupWebServer()` initializes HTTP endpoints
4. **First Request**: Browser makes any HTTP request (e.g., loading homepage)
5. **Validation Check**: Server detects partition state is `ESP_OTA_IMG_PENDING_VERIFY`
6. **Mark Valid**: Server calls `esp_ota_mark_app_valid_cancel_rollback()`
7. **Normal Operation**: Device continues running from new firmware

#### Rollback Process (Error Scenario)
1. **Bad Firmware**: Newly flashed firmware has bug or doesn't start web server
2. **No HTTP Request**: No HTTP request received within bootloader timeout (~30 sec)
3. **Automatic Rollback**: ESP32 bootloader switches back to previous partition
4. **Device Reboots**: Device reboots from old (working) firmware
5. **Normal Operation**: Device runs old firmware, OTA partition marked invalid

### API Endpoints

#### New Endpoints

**POST /api/firmware/upload**
- Purpose: Upload firmware binary file
- Method: POST
- Content-Type: `multipart/form-data`
- Request Body: Binary firmware file (.bin)
- Response Success (200):
  ```json
  {
    "status": "ok",
    "message": "Firmware uploaded successfully. Rebooting..."
  }
  ```
- Response Error (400):
  ```json
  {
    "error": "Invalid ESP32 firmware file"
  }
  ```
- Response Error (503):
  ```json
  {
    "error": "System busy. Try again later."
  }
  ```
- Response Error (500):
  ```json
  {
    "error": "Firmware update failed",
    "details": "Flash write error"
  }
  ```

**GET /firmware**
- Purpose: Serve firmware update page
- Method: GET
- Response: HTML page for firmware upload interface
- Response Type: `text/html`
- Fallback: 503 error if SD card unavailable

**GET /firmware.css**
- Purpose: Serve firmware page stylesheet
- Method: GET
- Response: CSS file
- Response Type: `text/css`

**GET /firmware.js**
- Purpose: Serve firmware upload JavaScript
- Method: GET
- Response: JavaScript file
- Response Type: `application/javascript`

#### Modified Endpoints

**All SD Card File Endpoints** (`/api/files/*`)
- Behavior change: Check if OTA upload is in progress
- If OTA active: Return 503 "System busy - firmware update in progress"
- Implementation: Check mutex availability before proceeding
- Affected endpoints:
  - `/api/files/upload`
  - `/api/files/write`
  - `/api/files/delete`
  - `/api/files/read`
  - `/api/files/mkdir`

**Health Endpoint** (`/api/health/status`)
- No changes required
- Continues to respond during OTA upload
- Could optionally add `"ota_in_progress": true` field to response

## Implementation Details

### Code Structure

#### Backend File Organization
```
src/
├── main.cpp                 # Modified: Add OTA endpoints, safe mode check
├── camera_config.h          # No changes
├── motion_detector.h/.cpp   # No changes
├── servo_controller.h/.cpp  # No changes
├── web_server.h/.cpp        # No changes
└── sd_manager.h/.cpp        # No changes
```

#### Frontend File Organization
```
data/web/
├── index.html              # Modified: Add firmware button in header
├── health.html             # Modified: Add firmware button in header
├── filemanager.html        # Modified: Add firmware button in header
├── firmware.html           # New: Firmware upload page
├── firmware.css            # New: Firmware page styles
├── firmware.js             # New: Firmware upload logic
├── style.css               # No changes
├── health.css              # No changes
├── filemanager.css         # No changes
└── [other files]           # No changes
```

#### PlatformIO Configuration
```ini
# platformio.ini - Modified

[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino

# Partition scheme - CHANGE THIS LINE:
# OLD: board_build.partitions = huge_app.csv
# NEW: board_build.partitions = min_spiffs.csv
# OR:  board_build.partitions = default_8MB.csv (if using 8MB flash)

# Alternatives (built-in ESP32 OTA schemes):
# - min_spiffs.csv: Factory + 2x OTA (1.9MB each) + SPIFFS (190KB)
# - default.csv: Factory + 2x OTA (1.3MB each) + SPIFFS (1.5MB)
# - default_8MB.csv: Factory + 2x OTA (3MB each) + SPIFFS (1.5MB)

# All other settings remain unchanged
```

### Libraries & Dependencies

#### Required ESP32 Libraries
- `Update.h` - ESP32 native OTA update library (built-in, no installation needed)
- `esp_ota_ops.h` - OTA partition management and rollback functions (built-in)
- `esp_partition.h` - Flash partition operations (built-in)

#### Existing Dependencies (No Changes)
- `ESPAsyncWebServer` - Already in use for web server
- `AsyncTCP` - Already in use for async networking
- `ArduinoJson` - Already in use for JSON responses
- `SD_MMC` - Already in use for SD card access
- `esp_camera` - Already in use for camera operations

### Key Algorithms

#### Magic Byte Validation
```cpp
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
```

#### OTA Upload Handler
```cpp
/**
 * OTA Firmware Upload Handler
 * Processes firmware binary in chunks and writes to OTA partition
 * Implements validation, error handling, and watchdog feeding
 */
server.on("/api/firmware/upload", HTTP_POST,
  // Response callback (executed after upload completes)
  [](AsyncWebServerRequest *request) {
    // Release SD card mutex
    if (otaUploadInProgress) {
      xSemaphoreGive(sdCardMutex);
      otaUploadInProgress = false;
    }

    // Check for errors
    if (Update.hasError()) {
      String error = "Update failed. Error: ";
      error += Update.errorString();
      Serial.println(error);
      request->send(500, "application/json",
        "{\"error\":\"" + error + "\"}");
      return;
    }

    // Success - send response and reboot
    Serial.println("OTA Update successful! Rebooting...");
    request->send(200, "application/json",
      "{\"status\":\"ok\",\"message\":\"Firmware updated. Rebooting...\"}");

    delay(1000); // Give time for response to be sent
    ESP.restart();
  },

  // Upload chunk callback (executed for each data chunk)
  [](AsyncWebServerRequest *request, String filename, size_t index,
     uint8_t *data, size_t len, bool final) {

    // First chunk - initialize OTA update
    if (index == 0) {
      Serial.printf("OTA Update started: %s\n", filename.c_str());

      // Acquire SD card mutex to block file operations
      if (xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        Serial.println("OTA Upload failed: SD card busy");
        return; // Mutex unavailable, upload will fail
      }
      otaUploadInProgress = true;

      // Validate ESP32 firmware format
      if (!isValidESP32Firmware(data, len)) {
        Serial.println("OTA Upload failed: Invalid firmware file");
        xSemaphoreGive(sdCardMutex);
        otaUploadInProgress = false;
        return; // Invalid firmware, upload will fail
      }

      // Begin OTA update
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        Serial.printf("OTA Update.begin() failed: %s\n", Update.errorString());
        xSemaphoreGive(sdCardMutex);
        otaUploadInProgress = false;
        return;
      }

      Serial.println("OTA Update initialized successfully");
    }

    // Write chunk to flash
    if (len) {
      size_t written = Update.write(data, len);
      if (written != len) {
        Serial.printf("OTA Write error: wrote %d of %d bytes\n", written, len);
        Update.abort();
        return;
      }

      // Feed watchdog every ~8KB to prevent timeout
      if (index % 8192 == 0) {
        delay(1);
      }

      // Log progress every 64KB
      if (index % 65536 == 0) {
        Serial.printf("OTA Progress: %d bytes written\n", index + len);
      }
    }

    // Final chunk - complete OTA update
    if (final) {
      if (Update.end(true)) {
        Serial.printf("OTA Update completed successfully: %d bytes\n", index + len);
      } else {
        Serial.printf("OTA Update.end() failed: %s\n", Update.errorString());
      }
    }
  }
);
```

#### Safe Mode Boot Validation
```cpp
/**
 * Safe Mode Boot Validation
 * Called on first HTTP request after boot to validate OTA update
 * Prevents rollback by marking new partition as valid
 */
bool firstRequestAfterBoot = true;

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

// In setupWebServer(), add middleware to all endpoints:
server.onNotFound([](AsyncWebServerRequest *request) {
  validateOTABoot();
  request->send(404, "text/plain", "Not found");
});

// Also call at start of main GET endpoints:
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  validateOTABoot();
  // ... rest of handler
});
```

### Memory Management

#### Upload Buffer Handling
- Chunk size determined by ESPAsyncWebServer (typically 1024-8192 bytes)
- No additional buffering beyond chunk buffer provided by server
- `Update.write()` writes directly to flash, no intermediate RAM buffer
- Maximum memory overhead: Single chunk buffer (~8KB)

#### Flash Partition Layout
```
# Example partition layout (min_spiffs.csv):
# Name      Type  SubType  Offset    Size      Flags
nvs         data  nvs      0x9000    0x5000
otadata     data  ota      0xe000    0x2000
app0        app   ota_0    0x10000   0x1E0000  # Slot 0 (1.9MB)
app1        app   ota_1    0x1F0000  0x1E0000  # Slot 1 (1.9MB)
spiffs      data  spiffs   0x3D0000  0x30000   # 190KB

# OTA Process:
# 1. Currently running from app0 (ota_0)
# 2. Update writes to app1 (ota_1)
# 3. otadata updated to point to app1
# 4. Reboot loads from app1
# 5. If successful, app0 becomes backup
# 6. Next update writes to app0, and so on
```

#### Watchdog Handling
- Feed watchdog every 8KB during write operations: `delay(1)`
- Prevents watchdog timeout during long flash write operations
- Critical for large firmware files (>1MB)

## Security & Safety

### Validation Mechanisms

#### Binary Format Validation
- **Magic Byte Check**: Verify first byte is 0xE9 (ESP32 firmware signature)
- **Timing**: Performed on first chunk, before calling `Update.begin()`
- **Failure Handling**: Reject upload immediately with clear error message
- **Purpose**: Prevent flashing of non-firmware files (images, text files, etc.)

#### Flash Write Validation
- **ESP32 Update Library**: Built-in CRC and integrity checks during write
- **Error Detection**: `Update.hasError()` checks for write failures
- **Automatic Abort**: Call `Update.abort()` on write errors to clean up state
- **Purpose**: Ensure firmware is written correctly to flash

### Rollback Protection

#### Automatic Rollback Mechanism
- **ESP32 Bootloader**: Built-in rollback support in ESP-IDF bootloader
- **Partition States**:
  - `ESP_OTA_IMG_PENDING_VERIFY`: New firmware needs validation
  - `ESP_OTA_IMG_VALID`: Firmware confirmed working
  - `ESP_OTA_IMG_INVALID`: Firmware failed, won't boot
- **Rollback Trigger**: If partition remains in `PENDING_VERIFY` state for ~30 seconds, bootloader reverts to previous partition
- **No User Action Required**: Fully automatic recovery from bad firmware

#### Safe Mode Validation
- **Success Criteria**: First successful HTTP request to any endpoint
- **Implementation**: Call `esp_ota_mark_app_valid_cancel_rollback()` on first request
- **Why HTTP Request**: Confirms web server started successfully and is responsive
- **Validation Window**: ~30 seconds after boot (bootloader timeout)

### Operational Safety

#### Mutex-Based Operation Blocking
- **SD Card Protection**: Acquire `sdCardMutex` during entire OTA process
- **Blocked Operations**: File upload, file write, file delete, mkdir
- **Allowed Operations**: Health check, status queries, camera stream (non-SD operations)
- **Purpose**: Prevent SD card corruption if concurrent write attempted during OTA

#### Single Upload Enforcement
- **Mechanism**: Static flag `otaUploadInProgress` tracks active upload
- **Check**: Reject new upload requests if flag is true
- **Purpose**: Prevent concurrent OTA uploads from corrupting flash

#### Graceful Error Recovery
- **Upload Failure**: Call `Update.abort()` to reset OTA state
- **Mutex Release**: Always release `sdCardMutex` in error paths
- **User Notification**: Return clear JSON error messages
- **Device State**: Device remains in previous firmware, fully functional

### Security Considerations (Current Scope)

#### No Authentication Required
- **Assumption**: Device operates on trusted network
- **Rationale**: Consistent with existing file upload, which also has no auth
- **Future Enhancement**: Add HTTP basic auth or API token validation

#### No Signature Verification
- **Current State**: No cryptographic verification of firmware authenticity
- **Risk**: Malicious firmware could be uploaded if attacker gains network access
- **Future Enhancement**: Implement signature verification with public key validation

#### Network Security
- **HTTP Only**: No HTTPS/TLS support currently
- **Risk**: Firmware could be intercepted or modified in transit
- **Mitigation**: Deploy on isolated/trusted networks only
- **Future Enhancement**: Add TLS support for encrypted firmware upload

## Testing Strategy

### Unit Tests (Manual Testing - No Test Framework Currently)

#### Binary Validation Tests
- **Test 1: Valid ESP32 Firmware**
  - Upload actual compiled .bin file from PlatformIO build
  - Expected: Magic byte validation passes, upload proceeds

- **Test 2: Invalid File (Text File)**
  - Rename .txt file to .bin and attempt upload
  - Expected: Magic byte validation fails with error message

- **Test 3: Invalid File (Image)**
  - Rename .jpg file to .bin and attempt upload
  - Expected: Magic byte validation fails with error message

- **Test 4: Empty File**
  - Create empty .bin file and attempt upload
  - Expected: Validation fails gracefully with error

#### Upload Handler Tests
- **Test 5: Small Firmware (<100KB)**
  - Upload small test firmware
  - Expected: Upload completes, device reboots, runs new firmware

- **Test 6: Large Firmware (~1.5MB)**
  - Upload typical full-featured firmware
  - Expected: Upload completes with progress updates, no timeout

- **Test 7: Maximum Size Firmware**
  - Upload firmware near partition size limit (1.9MB for min_spiffs)
  - Expected: Upload succeeds or fails gracefully with size error

#### Mutex Coordination Tests
- **Test 8: OTA During File Upload**
  - Start file upload, immediately attempt firmware upload
  - Expected: Firmware upload fails with "busy" error

- **Test 9: File Upload During OTA**
  - Start firmware upload, immediately attempt file upload
  - Expected: File upload fails with "busy" error

- **Test 10: Health Check During OTA**
  - Start firmware upload, query /api/health/status
  - Expected: Health endpoint responds successfully during upload

### Integration Tests

#### End-to-End Upload Flow
- **Test 11: Complete Upload Flow**
  - Select firmware via file input
  - Upload with progress monitoring
  - Wait for reboot
  - Verify device comes back online
  - Verify new firmware is running
  - Expected: Complete flow succeeds, device functional

- **Test 12: Drag-and-Drop Upload**
  - Drag .bin file to upload zone
  - Complete upload and reboot
  - Expected: Same success as file input method

- **Test 13: Multiple Sequential Uploads**
  - Upload firmware version A
  - Wait for boot, verify running
  - Upload firmware version B
  - Wait for boot, verify running
  - Expected: Multiple updates succeed sequentially

#### Error Recovery Tests
- **Test 14: Upload Interruption**
  - Start firmware upload
  - Disconnect WiFi mid-upload
  - Reconnect WiFi
  - Expected: Device remains functional on old firmware, SD card accessible

- **Test 15: Invalid Firmware Flash**
  - Modify valid .bin file to corrupt it (but keep magic byte)
  - Upload firmware
  - Expected: Update.end() fails, device remains on old firmware

#### Safe Mode & Rollback Tests
- **Test 16: Successful Boot Validation**
  - Upload valid firmware
  - Wait for reboot
  - Make HTTP request within 30 seconds
  - Check serial logs for "partition marked valid" message
  - Expected: Partition marked valid, no rollback

- **Test 17: Failed Boot Rollback** (Destructive Test)
  - Flash firmware with bug that prevents web server start
  - Wait 30+ seconds after reboot
  - Expected: Automatic rollback to previous firmware
  - **Note**: This test requires preparing intentionally broken firmware

### Edge Cases & Boundary Conditions

#### Concurrent Operation Tests
- **Test 18: Multiple Browser Connections**
  - Open firmware page in two browser tabs
  - Start upload from both simultaneously
  - Expected: One succeeds, other fails with busy/error

- **Test 19: Upload During Heavy Load**
  - Start camera streaming to multiple clients
  - Initiate firmware upload
  - Expected: Upload succeeds, camera stream may degrade but upload completes

#### Network Condition Tests
- **Test 20: Slow Network Upload**
  - Simulate slow WiFi (use router QoS or distance)
  - Upload firmware over slow connection
  - Expected: Upload completes slowly but successfully, no timeout

- **Test 21: Unstable Network**
  - Upload firmware with intermittent WiFi
  - Expected: Upload fails gracefully if connection lost, device remains functional

#### Partition & Storage Tests
- **Test 22: OTA from Different Partitions**
  - First update: Running from app0, write to app1
  - Second update: Running from app1, write to app0
  - Expected: Both update directions work correctly

- **Test 23: Full SPIFFS During OTA**
  - Fill SD card to capacity
  - Attempt firmware upload
  - Expected: OTA succeeds (uses flash partition, not SD card)

### User Interface Tests

#### Progress Feedback Tests
- **Test 24: Progress Bar Accuracy**
  - Upload firmware and monitor progress bar
  - Verify percentage matches upload progress
  - Expected: Progress bar moves smoothly from 0% to 100%

- **Test 25: Status Text Updates**
  - Monitor status text during upload
  - Verify stage messages appear: uploading → validating → flashing → rebooting
  - Expected: Status text reflects current operation accurately

#### Error Message Tests
- **Test 26: Invalid File Error Display**
  - Upload non-.bin file
  - Expected: Clear error message displayed in UI

- **Test 27: Upload Failure Error Display**
  - Disconnect during upload to trigger failure
  - Expected: Error message displayed, page remains usable

### Performance Tests

#### Timing Benchmarks
- **Test 28: 1MB Firmware Upload Time**
  - Upload 1MB firmware, measure total time
  - Target: <90 seconds total (upload + flash + reboot)

- **Test 29: Flash Write Speed**
  - Monitor serial logs during flash write
  - Calculate bytes/second write rate
  - Target: >20 KB/s write speed

#### Resource Monitoring
- **Test 30: Memory Usage During Upload**
  - Monitor heap memory via /api/health/status during upload
  - Expected: Free heap remains >30KB during upload

- **Test 31: CPU Load During Upload**
  - Monitor system responsiveness during upload
  - Query status endpoints during upload
  - Expected: System remains responsive, status queries succeed

### Regression Tests

#### Existing Functionality Preservation
- **Test 32: File Manager After OTA**
  - Perform firmware update
  - After reboot, verify file manager works
  - Expected: Upload, delete, edit files work normally

- **Test 33: Camera Stream After OTA**
  - Perform firmware update
  - After reboot, verify camera stream works
  - Expected: /stream endpoint works, video displays

- **Test 34: Object Tracking After OTA**
  - Perform firmware update
  - After reboot, test motion detection and servo control
  - Expected: Tracking functionality works normally

## Non-Goals (Out of Scope)

### Explicitly Excluded Features

#### MQTT-Based Remote Updates
- **Description**: Pushing firmware updates via MQTT message payload or URL
- **Rationale**: Adds complexity, requires MQTT broker setup, payload size limits
- **Future Consideration**: Could enable remote updates for deployed devices without web access

#### Automatic Update Checking
- **Description**: Device periodically checks remote server for new firmware versions
- **Rationale**: Requires update server infrastructure, version comparison logic
- **Future Consideration**: Useful for fleet management and automatic deployment

#### Version Comparison UI
- **Description**: Display current firmware version, compare with uploaded version, show changelog
- **Rationale**: Requires firmware metadata embedding, version parsing, storage
- **Future Consideration**: Improves user awareness of what's being updated

#### Firmware Metadata Validation
- **Description**: Validate version numbers, build dates, target hardware, dependencies
- **Rationale**: No metadata standard currently, would require firmware build changes
- **Future Consideration**: Prevents incompatible firmware uploads

#### Multiple Firmware Backup Retention
- **Description**: Store multiple previous firmware versions for rollback selection
- **Rationale**: Limited flash space, increases complexity, ESP32 supports only 2 OTA partitions
- **Future Consideration**: Could store backups on SD card with manual flash option

#### Cryptographic Signature Verification
- **Description**: Verify firmware is signed by trusted developer using RSA/ECDSA signatures
- **Rationale**: Requires key management, signing infrastructure, increases complexity
- **Future Consideration**: Critical for production deployments on untrusted networks

#### Scheduled or Delayed Updates
- **Description**: Schedule firmware updates for specific time, or delay until low-traffic period
- **Rationale**: Adds scheduling logic, requires RTC/NTP integration
- **Future Consideration**: Useful for minimizing disruption in production systems

#### Differential/Delta Updates
- **Description**: Upload only changed bytes instead of full firmware (binary diff patching)
- **Rationale**: Complex implementation, limited benefit for small ESP32 firmware sizes
- **Future Consideration**: Saves bandwidth for large firmware in slow network environments

#### Update Progress Persistence
- **Description**: Resume interrupted uploads from where they left off
- **Rationale**: Complex state management, flash wear concerns, low reliability benefit
- **Future Consideration**: Useful for very slow or unreliable network connections

#### Multi-Device Fleet Updates
- **Description**: Update multiple ESP32 devices simultaneously from single interface
- **Rationale**: Requires device discovery, coordination, monitoring infrastructure
- **Future Consideration**: Essential for managing large deployments

### Deferred to Future Iterations

#### Authentication/Authorization
- **Current State**: No authentication required (trusted network assumption)
- **Future Priority**: Medium - important for production but not prototyping
- **Implementation Path**: Add HTTP basic auth, then API tokens, then full user management

#### HTTPS/TLS Support
- **Current State**: HTTP only (unencrypted firmware upload)
- **Future Priority**: Medium - important for security but requires certificate management
- **Implementation Path**: Use self-signed certs for development, Let's Encrypt for production

#### Firmware Rollback UI
- **Current State**: Automatic rollback only on boot failure
- **Future Priority**: Low - manual rollback rarely needed with automatic protection
- **Implementation Path**: Add "revert to previous" button, store previous partition info

#### OTA Update Logging
- **Current State**: Serial logging only during development
- **Future Priority**: Low - useful for debugging but not essential
- **Implementation Path**: Store update history in JSON file on SD card

## Future Considerations

### Potential Enhancements

#### Version Management System
- Embed version metadata in firmware (build number, git hash, build date)
- Display current version on firmware page
- Compare versions before upload (prevent downgrade)
- Store version history in SD card JSON file

#### MQTT Remote Update Support
- Subscribe to MQTT topic for firmware update commands
- Download firmware from URL provided in MQTT message
- Report update progress via MQTT status messages
- Integrate with Home Assistant automation

#### Secure Boot & Signing
- Sign firmware binaries with private key during build
- Embed public key in bootloader
- Verify signature before flashing
- Reject unsigned or tampered firmware

#### Update Scheduling
- Schedule updates for specific time window
- Delay update until device idle (no motion detection active)
- Automatic retry on failure with exponential backoff

#### Backup & Restore
- Save current firmware to SD card before update
- Manual restore from SD card backup via web UI
- Backup rotation (keep last N versions)

#### Advanced Progress Feedback
- Estimated time remaining calculation
- Upload speed display (KB/s)
- Detailed stage breakdown with timing
- Post-update validation report

#### Multi-Partition Support
- Support for custom partition schemes
- Configurable partition selection
- Factory partition reset option

### Integration Opportunities

#### Home Assistant Integration
- Entity for firmware version sensor
- Update available binary sensor
- OTA service call for remote update
- Update progress sensor with percentage

#### CI/CD Pipeline Integration
- Automatic firmware upload on git push
- PlatformIO build → upload via API
- Automated testing after deployment
- Rollback on test failure

#### Cloud Storage Integration
- Upload firmware to cloud storage (S3, Google Cloud Storage)
- Device downloads firmware from cloud URL
- Version manifest hosted in cloud
- Reduces local network bandwidth usage

#### Monitoring & Alerting
- OTA success/failure logging to cloud
- Email/SMS notification on update completion
- Rollback alerts for failed updates
- Update history dashboard

---

**Document Version**: 1.0
**Date**: 2025-11-17
**Author**: Spec Writer Agent
**Status**: Ready for Implementation
