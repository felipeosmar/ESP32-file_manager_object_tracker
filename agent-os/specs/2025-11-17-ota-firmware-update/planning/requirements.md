# Spec Requirements: OTA Firmware Update

## Initial Description
Over-the-air firmware update support - This will allow users to update the ESP32 firmware remotely via the web interface without requiring physical access to the device or USB cable connection.

## Requirements Discussion

### First Round Questions

**Q1: Which OTA implementation approach should we use?**
**Answer:** ESP32's native Update class with HTTP upload through ESPAsyncWebServer (similar to current file uploads)

**Q2: What partition scheme should we use?**
**Answer:** Two-partition OTA scheme (factory + ota_0 + ota_1) with automatic rollback on boot failure

**Q3: Should the firmware upload interface be integrated into the file manager or be a separate dedicated page?**
**Answer:** Separate /firmware page with its own route, accessible via button in header (like filemanager and health)

**Q4: What security measures should we implement for firmware uploads?**
**Answer:** Open for now on trusted networks (no authentication required initially)

**Q5: What should the firmware upload workflow look like from a user perspective?**
**Answer:** select .bin file → upload → validate → write to OTA partition → auto-reboot. Show progress bar and prevent navigation during updates

**Q6: Should we implement firmware version tracking or validation before accepting uploads?**
**Answer:** Just validate binary format (no metadata validation)

**Q7: How should we handle errors during the OTA update process?**
**Answer:** Implement "safe mode" boot - wait 30 seconds after OTA update to ensure successful connection before marking partition as valid

**Q8: Should firmware files be stored on SD card temporarily before flashing, or written directly to OTA partition?**
**Answer:** Write directly to OTA partition (not SD card). Block other operations during upload

**Q9: What features should NOT be included in this initial implementation?**
**Answer:**
- MQTT-based remote updates
- Automatic update checking
- Version comparison UI
- Multiple firmware backup retention

### Follow-up Questions

**Follow-up 1: Safe Mode Success Criteria**
**Question:** What specific criteria should define a "successful boot" after an OTA update? Should we wait for the first HTTP request, MQTT connection, camera initialization, or some other signal?
**Answer:** First successful HTTP request to any endpoint (confirms web server is running and responsive after update)

**Follow-up 2: Upload UI Design Reusability**
**Question:** Should the firmware upload page reuse the exact same visual design as the file manager (with drag-and-drop zone), or should it have a simpler, single-purpose interface?
**Answer:** Reuse the same design as file manager (drag-and-drop zone, file input, XHR with progress bar)

**Follow-up 3: Partition Scheme Configuration**
**Question:** For the partition scheme modification - should we use ESP32's built-in OTA partition schemes (like min_spiffs.csv) or create a custom partition table?
**Answer:** Use ESP32's built-in OTA partition scheme (like min_spiffs.csv)

**Follow-up 4: Binary Format Validation**
**Question:** What specific binary format validation should we perform beyond checking file extension? Should we verify ESP32 magic bytes, check minimum file size, or validate partition compatibility?
**Answer:** Check ESP32 magic byte (0xE9) at start of file

**Follow-up 5: Operation Blocking During Upload**
**Question:** When you say "block other operations during upload" - should we block ALL endpoints (making the device completely unresponsive except for the upload endpoint), or just block file operations (SD card access) while allowing status checks and health monitoring?
**Answer:** Block only file operations (SD card), not all endpoints

### Existing Code to Reference

**Similar Features Identified:**
Based on code analysis, the following existing patterns should be referenced:

- **File Upload Pattern**: `/src/main.cpp` lines 644-709 - ESPAsyncWebServer upload handler with chunked processing
  - Uses `AsyncWebServerRequest` with file upload callback
  - Implements progress tracking via data chunks
  - Includes watchdog feeding for large uploads
  - Pattern: `server.on("/api/files/upload", HTTP_POST, [response handler], [upload handler])`

- **Frontend Upload UI**: `/data/web/filemanager.html` and `/data/web/filemanager.js` lines 165-244
  - File selection via input element
  - Progress bar visualization
  - XHR-based upload with progress events
  - Confirmation dialogs for overwrite protection

- **Page Navigation**: `/data/web/index.html` and `/data/web/health.html`
  - Header with navigation buttons to different pages
  - Pattern: `<a href="/health" class="filemanager-btn">🏥 Saúde</a>`
  - Consistent header layout across pages

- **SD Card Mutex Protection**: `/src/main.cpp` lines 36, 63-66
  - Global `SemaphoreHandle_t sdCardMutex` for thread-safe operations
  - Similar pattern should be used to block SD operations during OTA

### Visual Assets

No visual assets provided.

## Requirements Summary

### Functional Requirements

**Core OTA Functionality:**
- Use ESP32's native `Update` class for firmware updates
- Implement HTTP-based upload through ESPAsyncWebServer
- Write firmware directly to OTA partition (no SD card intermediate storage)
- Support two-partition OTA scheme using ESP32's built-in partition scheme (min_spiffs.csv)
- Automatic rollback on boot failure
- Auto-reboot after successful firmware write
- Validate ESP32 binary format by checking magic byte (0xE9) at file start

**User Interface:**
- Create dedicated `/firmware` page (separate from file manager)
- Add navigation button in header (similar to "Arquivos" and "Saúde" buttons)
- Reuse file manager's upload design:
  - Drag-and-drop zone for .bin files
  - File input element
  - XHR-based upload with progress tracking
- Real-time upload progress bar
- Prevent navigation during active upload/update
- Visual feedback for upload status (uploading, validating, flashing, rebooting)

**Upload Workflow:**
1. User selects .bin firmware file (via file input or drag-and-drop)
2. Validate ESP32 magic byte (0xE9) at start of file
3. Upload begins with progress indication
4. Write directly to OTA partition (streaming, no buffering)
5. Automatic reboot on completion

**Validation:**
- Check ESP32 magic byte (0xE9) at start of uploaded file
- Reject files that don't match ESP32 binary format
- No metadata or version checking required
- No signature verification required initially

**Error Handling & Safe Mode:**
- Implement "safe mode" boot mechanism after OTA update
- Success criteria: First successful HTTP request to any endpoint after reboot
- If no successful HTTP request is received, automatic rollback occurs
- User-facing error messages for failed uploads
- Abort mechanism if upload fails mid-process

**Operational Constraints:**
- Block only SD card file operations during firmware upload (not all endpoints)
- Allow health monitoring and status checks during upload
- Use mutex pattern (similar to SD card mutex) to prevent file operations
- No authentication required (trusted network assumption)

### Reusability Opportunities

**Upload Handler Pattern:**
- Reuse chunked upload handler structure from file upload (`/api/files/upload`)
- Adapt progress tracking mechanism for firmware upload
- Similar watchdog feeding pattern for large file processing
- Replace file write operations with `Update.write()` calls

**Frontend Components:**
- Reuse progress bar HTML/CSS from filemanager
- Reuse drag-and-drop zone implementation
- Adapt XHR upload code with progress events
- Similar page structure and header navigation pattern

**Mutex Pattern:**
- Adapt SD card mutex pattern for blocking file operations during OTA
- Allow concurrent access to non-file endpoints

### Scope Boundaries

**In Scope:**
- Web-based firmware upload interface with drag-and-drop
- ESP32 native OTA implementation using Update library
- Two-partition scheme with rollback protection using built-in partition scheme
- ESP32 magic byte (0xE9) validation
- Progress indication and user feedback
- Automatic reboot after successful update
- Safe mode boot mechanism with HTTP request validation
- Blocking SD card operations during update (while allowing other endpoints)
- PlatformIO configuration changes for OTA partition scheme

**Out of Scope:**
- MQTT-based remote firmware updates
- Automatic update checking from remote server
- Version comparison UI
- Firmware metadata validation (version numbers, build dates, etc.)
- Multiple firmware backup retention
- Cryptographic signature verification
- Over-the-air configuration of partition scheme
- Update scheduling or delayed updates
- Authentication/authorization for firmware uploads
- Blocking all endpoints during update (only file operations blocked)

### Technical Considerations

**ESP32 Libraries Required:**
- `Update.h` - ESP32 native OTA update library
- `esp_ota_ops.h` - For partition management and rollback
- `esp_partition.h` - For partition scheme operations

**Integration Points:**
- ESPAsyncWebServer for HTTP upload endpoint (`/api/firmware/upload`)
- Existing web interface navigation structure
- PlatformIO partition scheme configuration (`platformio.ini`)
- Modify to use built-in OTA partition scheme (min_spiffs.csv or similar)

**Existing System Constraints:**
- Running on ESP32-CAM with limited memory (520KB SRAM, 4MB Flash)
- Current partition scheme needs modification to support OTA
- ESPAsyncWebServer already handling file uploads
- Watchdog timer considerations during long operations
- SD card mutex pattern for operation blocking

**Technology Stack:**
- Backend: C++/Arduino framework with ESP32 Update library
- Frontend: Vanilla HTML/CSS/JavaScript (no frameworks)
- Build System: PlatformIO with espressif32 platform
- Web Server: ESPAsyncWebServer (asynchronous, non-blocking)

**Performance Considerations:**
- Firmware files typically 1-2MB for ESP32 applications
- Upload will take longer than typical file uploads
- Must prevent system timeout during validation and flashing
- Memory constraints require streaming write (not buffering entire file)
- Magic byte validation happens at start of upload (minimal overhead)

**Binary Format Validation Details:**
- ESP32 binary format starts with magic byte: 0xE9
- Validation should happen before accepting the upload
- Reject non-ESP32 binaries immediately to save bandwidth/time

**Safe Mode Implementation:**
- After OTA update and reboot, device enters "validation mode"
- First successful HTTP request to ANY endpoint marks boot as successful
- System calls `esp_ota_mark_app_valid_cancel_rollback()` on first HTTP success
- If device fails to boot or web server doesn't start, automatic rollback occurs
- No user intervention required for rollback

**Operation Blocking Strategy:**
- Create or reuse mutex for SD card operations
- Block file upload, download, delete, and other SD operations during OTA
- Allow health check endpoint to continue responding
- Allow status queries and other non-file endpoints
- Release mutex after OTA completion (successful or failed)
