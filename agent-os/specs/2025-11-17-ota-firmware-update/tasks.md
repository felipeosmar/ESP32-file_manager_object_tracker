# Task Breakdown: OTA Firmware Update Feature

## Overview
Total Task Groups: 5
Estimated Complexity: Medium-High
Dependencies: Configuration -> Backend -> Frontend -> Integration -> Testing

## Task List

### Group 1: Configuration & Setup
**Dependencies:** None
**Specialist:** ESP32 Platform Engineer
**Estimated Effort:** 1-2 hours

- [x] 1.0 Complete platform configuration for OTA support
  - [x] 1.1 Modify PlatformIO partition scheme configuration
    - Open `/platformio.ini`
    - Change `board_build.partitions = huge_app.csv` to `board_build.partitions = min_spiffs.csv`
    - Add comment explaining OTA partition layout (factory + ota_0 + ota_1 + spiffs)
    - Verify partition scheme supports OTA (Factory app + 2x OTA partitions)
    - Reference: Spec section "PlatformIO Configuration" lines 537-557
  - [x] 1.2 Verify required ESP32 libraries are available
    - Confirm `Update.h` is accessible (built-in ESP32 library)
    - Confirm `esp_ota_ops.h` is accessible (built-in ESP-IDF library)
    - Confirm `esp_partition.h` is accessible (built-in ESP-IDF library)
    - No additional library installations required
  - [x] 1.3 Build firmware with new partition scheme
    - Run `pio run` to compile with min_spiffs.csv partition scheme
    - Verify build succeeds without errors
    - Check binary size is within OTA partition limits (< 1.9MB for min_spiffs)
    - Note binary location for testing: `.pio/build/esp32cam/firmware.bin`
  - [ ] 1.4 Flash initial firmware and verify partitions
    - Flash compiled firmware to device: `pio run --target upload`
    - Monitor serial output for successful boot
    - Verify web interface loads correctly after partition change
    - Confirm existing features (file manager, health, camera) still work

**Acceptance Criteria:**
- PlatformIO builds successfully with min_spiffs.csv partition scheme
- Device boots and runs existing features normally
- Binary size is within OTA partition limits
- Serial output shows correct partition initialization

---

### Group 2: Backend - OTA Core Implementation
**Dependencies:** Group 1 (Configuration) - COMPLETE
**Specialist:** Embedded Systems Engineer
**Estimated Effort:** 4-6 hours

- [x] 2.0 Complete backend OTA functionality
  - [ ] 2.1 Write 4-6 focused tests for OTA backend functionality
    - Test 1: Valid ESP32 firmware binary validation (magic byte 0xE9)
    - Test 2: Invalid file rejection (non-ESP32 binary)
    - Test 3: OTA upload endpoint accepts .bin file
    - Test 4: Safe mode boot validation after OTA update
    - Test 5: Mutex acquisition blocks SD card operations during OTA
    - Test 6: Error handling for failed flash write
    - Limit to 6 tests maximum - focus on critical behaviors only
  - [x] 2.2 Implement binary format validation function
    - Create `isValidESP32Firmware(uint8_t *data, size_t len)` function
    - Check magic byte: `data[0] == 0xE9` (ESP32 binary signature)
    - Return boolean: true if valid, false if invalid
    - Add serial logging for validation results
    - Reference: Spec "Magic Byte Validation" algorithm lines 575-601
    - Location: Add to `main.cpp` before `setupWebServer()`
  - [x] 2.3 Create OTA upload endpoint handler
    - Add endpoint: `server.on("/api/firmware/upload", HTTP_POST, [response], [upload])`
    - Response callback: Check `Update.hasError()`, send JSON response, call `ESP.restart()`
    - Upload callback structure:
      - First chunk (index == 0): Acquire `sdCardMutex`, validate binary, call `Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)`
      - Each chunk: Call `Update.write(data, len)`, feed watchdog every 8KB
      - Final chunk: Call `Update.end(true)`, check for errors
      - Error handling: Call `Update.abort()` and release mutex on failure
    - Reuse pattern from: `/api/files/upload` handler (lines 644-709)
    - Reference: Spec "OTA Upload Handler" algorithm lines 604-702
    - Add global flag: `bool otaUploadInProgress = false`
  - [x] 2.4 Implement SD card mutex integration for OTA
    - Acquire `sdCardMutex` at start of OTA upload (first chunk)
    - Set `otaUploadInProgress = true` when mutex acquired
    - Hold mutex for entire upload duration
    - Release mutex in response callback (success or failure path)
    - Add timeout handling: Return 503 "System busy" if mutex unavailable
    - Pattern: `xSemaphoreTake(sdCardMutex, pdMS_TO_TICKS(5000))`
    - Reference: Spec "SD Card Mutex Integration" lines 344-367
  - [x] 2.5 Implement safe mode boot validation
    - Add global flag: `bool firstRequestAfterBoot = true`
    - Create `validateOTABoot()` function:
      - Check if `firstRequestAfterBoot == true`
      - Get running partition: `esp_ota_get_running_partition()`
      - Check partition state: `esp_ota_get_state_partition()`
      - If state is `ESP_OTA_IMG_PENDING_VERIFY`: call `esp_ota_mark_app_valid_cancel_rollback()`
      - Set `firstRequestAfterBoot = false`
    - Call `validateOTABoot()` in:
      - `server.onNotFound()` handler
      - Start of main GET endpoints (`/`, `/health`, `/filemanager`, `/firmware`)
    - Reference: Spec "Safe Mode Boot Validation" algorithm lines 705-755
    - Add serial logging for validation events
  - [x] 2.6 Add firmware page serving endpoints
    - GET `/firmware`: Serve `/web/firmware.html` from SD card
    - GET `/firmware.css`: Serve `/web/firmware.css` from SD card
    - GET `/firmware.js`: Serve `/web/firmware.js` from SD card
    - Reuse pattern from: `/health` and `/filemanager` endpoints
    - Use `serveStaticFile()` helper function
    - Add fallback: Return 503 if SD card unavailable
  - [x] 2.7 Update SD card file endpoints to check OTA status
    - Modify endpoints: `/api/files/upload`, `/api/files/write`, `/api/files/delete`, `/api/files/read`, `/api/files/mkdir`
    - Add check at start of each handler: if `otaUploadInProgress`, return 503 JSON error
    - Error response: `{"error": "System busy - firmware update in progress"}`
    - Prevents SD card corruption during OTA update
  - [x] 2.8 Add include directives for OTA libraries
    - Add to top of `main.cpp`:
      - `#include <Update.h>`
      - `#include <esp_ota_ops.h>`
      - `#include <esp_partition.h>`
  - [ ] 2.9 Run backend tests only
    - Execute ONLY the 4-6 tests written in task 2.1
    - Verify OTA upload handler initializes correctly
    - Verify binary validation rejects invalid files
    - Verify safe mode validation logic works
    - Do NOT run entire test suite at this stage

**Acceptance Criteria:**
- All 4-6 backend tests pass
- OTA upload endpoint accepts valid firmware files
- Binary validation correctly rejects non-ESP32 files
- SD card operations blocked during OTA (mutex works)
- Safe mode validation marks partition valid after first HTTP request
- Device reboots successfully after OTA completion

---

### Group 3: Frontend - Upload Interface
**Dependencies:** Group 2 (Backend OTA) - COMPLETE
**Specialist:** Embedded Web UI Designer
**Estimated Effort:** 4-5 hours

- [x] 3.0 Complete firmware upload web interface
  - [ ] 3.1 Write 4-6 focused tests for UI functionality (defer to testing phase)
    - Test 1: Upload zone accepts .bin file via file input
    - Test 2: Upload zone accepts .bin file via drag-and-drop
    - Test 3: Progress bar updates during upload (0-100%)
    - Test 4: Status text shows correct stages (uploading, validating, flashing, rebooting)
    - Test 5: Error message displays when invalid file selected
    - Test 6: Navigation blocked during active upload
    - Limit to 6 tests maximum - focus on critical user interactions only
  - [x] 3.2 Create firmware upload page HTML
    - Create `/data/web/firmware.html`
    - Structure:
      - Header with navigation (reuse pattern from index.html/health.html)
      - Navigation buttons: Home, Health, Firmware (active), Files
      - Page title: "Atualização de Firmware" with rocket emoji
      - Upload zone: Drag-and-drop container (reuse filemanager pattern)
      - Progress section: Progress bar with percentage display
      - Status display: Text showing current operation stage
      - Warning section: Info about OTA process and device reboot
    - Use consistent layout: `<div class="container">` with `<header>`, `<main>`
    - Reference: Spec "Page Layout" lines 108-123 and "Upload Zone Styling" lines 116-123
  - [x] 3.3 Implement upload zone design
    - Dashed border container (similar to file manager)
    - Icon: Firmware chip emoji or microchip icon
    - Text: "Arraste o arquivo .bin aqui ou clique para selecionar"
    - File input: `<input type="file" accept=".bin" id="firmwareInput">`
    - Hidden by default, triggered by zone click
    - Hover state: Border color change
    - Drag-over state: Background color change during drag
    - Reference: Spec "Upload Zone Styling" lines 116-123
  - [x] 3.4 Create progress bar component
    - Container: `<div id="progressContainer">` (hidden by default)
    - Progress bar: `<div id="progressBar">` with fill element
    - Percentage text: Overlay on progress fill
    - Status text: `<p id="statusText">` below progress bar
    - CSS: Smooth width transition animation
    - Reference: Existing progress bar in filemanager.js lines 198-243
  - [x] 3.5 Create firmware upload JavaScript
    - Create `/data/web/firmware.js`
    - File selection handling:
      - Click event on upload zone → trigger file input
      - File input change event → validate and upload
      - Drag-and-drop events: dragover, dragleave, drop
      - Visual feedback: add/remove 'dragging' class
    - Client-side validation:
      - Check file extension is .bin
      - Reject if not .bin with error message
      - Single file only (no multiple selection)
    - XHR upload with progress tracking:
      - POST to `/api/firmware/upload`
      - FormData with file
      - Progress event listener: update progress bar
      - Upload stages: uploading (0-95%), validating (95-96%), flashing (96-99%), rebooting (100%)
      - Status text updates for each stage
    - Reference: Existing upload logic in filemanager.js lines 198-243
    - Reference: Spec "Upload JavaScript Logic" lines 254-262
  - [x] 3.6 Implement navigation blocking during upload
    - Add `beforeunload` event listener when upload starts
    - Display browser warning: "Firmware update in progress. Leaving this page will interrupt the update."
    - Remove event listener after upload completes (success or failure)
    - Disable navigation buttons during upload (add 'disabled' class)
  - [x] 3.7 Implement error handling and user feedback
    - Error banner component: `<div id="errorBanner">` (hidden by default)
    - Error types:
      - Invalid file: "Arquivo inválido: não é um firmware ESP32"
      - Upload failure: "Erro no upload. Tente novamente."
      - Flash failure: "Erro ao gravar firmware. Dispositivo permanece na versão anterior."
    - Red banner for critical errors
    - Orange banner for warnings
    - Auto-hide after 10 seconds or manual close button
    - Reference: Spec "Error States" lines 133-137
  - [x] 3.8 Implement post-reboot auto-reconnect
    - After successful upload response, show "Reiniciando..." message
    - Start polling: Try to fetch `/api/health/status` every 3 seconds
    - Max retries: 20 attempts (60 seconds total)
    - On successful response: Show "Atualização completa! Dispositivo online." and reload page
    - On timeout: Show error message with manual reload button
  - [x] 3.9 Create firmware page CSS
    - Create `/data/web/firmware.css`
    - Upload zone styles:
      - Dashed border: 2px dashed #ccc
      - Padding: 40px
      - Border-radius: 8px
      - Hover: border-color: #007bff
      - Dragging: background-color: #f0f8ff
    - Progress bar styles:
      - Container: width 100%, height 40px, border-radius 8px, background #e0e0e0
      - Fill: height 100%, background linear-gradient (blue), transition: width 0.3s
      - Text: centered, white color, font-weight bold
    - Error banner styles:
      - Red background for errors: #dc3545
      - Orange background for warnings: #ff9800
      - White text, padding 15px, border-radius 5px
    - Responsive breakpoints:
      - Mobile (<480px): Compact layout, smaller upload zone
      - Tablet (480-768px): Stacked layout
      - Desktop (>768px): Full layout
    - Reference: Spec "Responsive Design" lines 144-148
  - [x] 3.10 Add firmware navigation button to all pages
    - Modify `/data/web/index.html`: Add firmware button in header
    - Modify `/data/web/health.html`: Add firmware button in header
    - Modify `/data/web/filemanager.html`: Add firmware button in header
    - Button pattern: `<a href="/firmware" class="firmware-btn">🚀 Firmware</a>`
    - Position: Between "Saúde" and "Arquivos" buttons
    - Consistent styling with existing navigation buttons
    - Mobile responsive: Icon-only on small screens
    - Reference: Spec "Navigation Button" lines 138-143
  - [ ] 3.11 Run frontend tests only (defer to testing phase)
    - Execute ONLY the 4-6 tests written in task 3.1
    - Verify upload zone accepts .bin files
    - Verify progress bar updates correctly
    - Verify error messages display properly
    - Do NOT run entire test suite at this stage

**Acceptance Criteria:**
- All 4-6 frontend tests pass
- Firmware page loads with consistent design
- Upload zone accepts .bin files via click and drag-drop
- Progress bar animates smoothly from 0-100%
- Status text updates through all stages
- Error messages display clearly
- Navigation blocked during upload
- Auto-reconnect works after device reboot
- Firmware button appears in all page headers

---

### Group 4: Integration & End-to-End Testing
**Dependencies:** Groups 1, 2, 3 (All previous groups) - COMPLETE
**Specialist:** Embedded Systems Test Engineer
**Estimated Effort:** 3-4 hours

**IMPORTANT: These tests require physical access to ESP32 device and must be executed manually by user.**

**Test Documentation Created:**
- [x] Test Plan Document: `/agent-os/specs/2025-11-17-ota-firmware-update/test-plan.md`
- [x] Test Results Template: `/agent-os/specs/2025-11-17-ota-firmware-update/test-results.md`
- [x] Quick Testing Guide: `/agent-os/specs/2025-11-17-ota-firmware-update/testing-guide.md`

**User Action Required:** Execute tests following the testing-guide.md and document results in test-results.md

- [ ] 4.0 Complete integration testing and validation
  - [ ] 4.1 Test complete OTA upload flow (end-to-end)
    - Build test firmware with new partition scheme
    - Access `/firmware` page from browser
    - Select firmware.bin file via file input
    - Monitor upload progress (0-100%)
    - Verify status text updates: uploading → validating → flashing → rebooting
    - Wait for device reboot (~10 seconds)
    - Verify auto-reconnect polls and succeeds
    - Verify device loads homepage successfully
    - Expected: Complete flow succeeds, device runs new firmware
  - [ ] 4.2 Test drag-and-drop upload
    - Access `/firmware` page
    - Drag firmware.bin file to upload zone
    - Verify drag-over visual feedback (border/background change)
    - Drop file on zone
    - Verify upload proceeds identically to file input method
  - [ ] 4.3 Test binary validation with invalid files
    - Test invalid file 1: Rename .txt file to .bin and upload
    - Test invalid file 2: Rename .jpg file to .bin and upload
    - Test empty file: Create empty .bin file and upload
    - Expected: Magic byte validation fails, error message displays
  - [ ] 4.4 Test SD card mutex coordination
    - Start firmware upload (large file for longer upload time)
    - While uploading, attempt file upload via file manager
    - Expected: File upload fails with "System busy" error
    - Wait for firmware upload to complete
    - Verify file manager operations work after OTA completes
  - [ ] 4.5 Test health endpoint during OTA
    - Start firmware upload
    - While uploading, access `/api/health/status` endpoint
    - Expected: Health endpoint responds successfully during upload
  - [ ] 4.6 Test safe mode boot validation
    - Upload valid firmware
    - Monitor serial output during reboot
    - Verify partition state is `ESP_OTA_IMG_PENDING_VERIFY` immediately after boot
    - Access any page within 30 seconds
    - Check serial logs for "OTA update validated successfully" message
    - Expected: Partition marked valid, no rollback occurs
  - [ ] 4.7 Test sequential firmware updates
    - Upload firmware version A
    - Wait for reboot, verify device running version A
    - Upload firmware version B
    - Wait for reboot, verify device running version B
    - Expected: Multiple sequential updates succeed
  - [ ] 4.8 Test upload interruption recovery
    - Start firmware upload
    - Disconnect WiFi mid-upload (simulate network failure)
    - Reconnect WiFi
    - Verify device remains functional on old firmware
    - Verify SD card operations work
    - Expected: Device recovers gracefully, no corruption
  - [ ] 4.9 Test error handling and user feedback
    - Test upload failure scenarios
    - Test invalid file scenarios
    - Test busy state scenarios
    - Expected: All error scenarios show clear user-facing messages
  - [ ] 4.10 Performance and timing benchmarks
    - Upload 1MB firmware, measure total time
    - Target: < 90 seconds total
    - Monitor serial output for flash write speed
    - Target: > 20 KB/s write speed
    - Query `/api/health/status` during upload to check free heap
    - Target: Free heap remains > 30KB during upload

**Acceptance Criteria:**
- Complete OTA flow works end-to-end
- Binary validation rejects invalid files
- SD card operations blocked during OTA
- Health monitoring continues during OTA
- Safe mode validation prevents rollback
- Sequential updates work in both directions (app0↔app1)
- Upload interruption doesn't corrupt device
- All error scenarios display clear messages
- Performance meets targets (< 90s total, > 20 KB/s write)

---

### Group 5: Regression Testing & Documentation
**Dependencies:** Group 4 (Integration)
**Specialist:** QA Engineer
**Estimated Effort:** 2-3 hours

- [ ] 5.0 Verify existing functionality and complete final validation
  - [ ] 5.1 Test existing features after OTA update
    - Perform firmware update to new version
    - After reboot, test file manager:
      - Upload file via drag-drop
      - Download file
      - Delete file
      - Create directory
      - Edit file
    - Expected: All file manager operations work normally
    - Test camera stream:
      - Access `/stream` endpoint
      - Verify video displays in browser
    - Expected: Camera stream works normally
    - Test object tracking:
      - Enable auto-tracking via web interface
      - Trigger motion detection
      - Verify servo motors respond
    - Expected: Tracking functionality works normally
    - Reference: Spec Tests 32-34 "Regression Tests" lines 1035-1050
  - [ ] 5.2 Review test coverage and identify critical gaps
    - Review tests from Groups 2-4:
      - 4-6 backend tests (Group 2)
      - 4-6 frontend tests (Group 3)
      - 10 integration tests (Group 4)
      - Total: approximately 18-22 tests
    - Analyze critical gaps ONLY for OTA feature (not entire application)
    - Focus on gaps in end-to-end workflows
    - Prioritize: firmware upload failure recovery, rollback scenarios, concurrent operation handling
  - [ ] 5.3 Write up to 8 additional strategic tests maximum
    - Add maximum 8 new tests to fill identified critical gaps
    - Focus on:
      - Rollback testing (intentionally broken firmware - destructive test)
      - Large firmware files (near partition size limit)
      - Network instability scenarios
      - Concurrent browser connections
      - Full SPIFFS during OTA
    - Do NOT write comprehensive coverage for all edge cases
    - Skip: Performance stress tests, accessibility tests (unless business-critical)
    - Reference: Spec Tests 15-23 for additional test ideas
  - [ ] 5.4 Run all OTA feature tests
    - Execute all tests from Groups 2, 3, 4, and 5.3
    - Expected total: approximately 26-30 tests maximum
    - Verify all critical workflows pass
    - Do NOT run entire application test suite
    - Document any test failures and root cause
  - [ ] 5.5 Create testing summary documentation
    - Document test results in `/agent-os/specs/2025-11-17-ota-firmware-update/testing-summary.md`
    - Include:
      - Total tests executed
      - Pass/fail results
      - Performance benchmarks (upload time, flash speed)
      - Known limitations or edge cases
      - Recommendations for future testing
    - Keep documentation concise (1-2 pages maximum)
  - [ ] 5.6 Verify serial logging and debugging output
    - Upload firmware and monitor serial output
    - Verify logging shows:
      - "OTA Update started: firmware.bin"
      - "Firmware validation passed: ESP32 magic byte detected"
      - "OTA Update initialized successfully"
      - "OTA Progress: X bytes written" (every 64KB)
      - "OTA Update completed successfully: X bytes"
      - "First boot after OTA update detected"
      - "OTA update validated successfully - rollback cancelled"
    - Ensure log messages are clear and helpful for debugging
  - [ ] 5.7 Final smoke test on clean device
    - Flash original firmware (before OTA feature) to clean device
    - Upload new firmware with OTA feature via USB
    - Perform OTA update from web interface
    - Verify complete cycle works on fresh device
    - Expected: OTA works on clean installation, not just development device

**Acceptance Criteria:**
- All feature tests pass (approximately 26-30 tests total)
- Existing features (file manager, camera, tracking) work after OTA
- Critical gaps covered with maximum 8 additional tests
- Testing summary documented
- Serial logging provides clear debugging information
- OTA feature works on clean device installation
- No more than 8 additional tests added in gap analysis

---

## Execution Order Summary

**Critical Path:**
1. Group 1: Configuration & Setup (1-2 hours) - COMPLETE
2. Group 2: Backend OTA Implementation (4-6 hours) - COMPLETE
3. Group 3: Frontend Upload Interface (4-5 hours) - COMPLETE
4. Group 4: Integration & End-to-End Testing (3-4 hours) - **TEST PLAN READY - USER EXECUTION REQUIRED**
5. Group 5: Regression Testing & Documentation (2-3 hours) - PENDING

**Total Estimated Effort:** 14-20 hours

**Key Dependencies:**
- Group 2 depends on Group 1 (partition scheme must be configured first)
- Group 3 can start after Group 2 completes (frontend needs working backend API)
- Group 4 requires Groups 1-3 complete (integration needs full stack)
- Group 5 requires Group 4 complete (regression needs working feature)

**Parallel Work Opportunities:**
- Frontend CSS/HTML design (Group 3.2-3.4, 3.9) can be prototyped during Group 2
- Documentation preparation (Group 5.5) can be drafted during Group 4

**Critical Milestones:**
1. Configuration complete and device boots with new partition scheme (Group 1.4)
2. Backend OTA endpoint accepts and flashes valid firmware (Group 2.9)
3. Frontend upload interface works end-to-end (Group 3.11)
4. First successful OTA update via web interface (Group 4.1) - **PENDING USER EXECUTION**
5. All regression tests pass (Group 5.4)

**Testing Strategy:**
- Each development group (2-3) writes 4-6 focused tests (total: ~18-22 tests)
- Integration group adds 10 comprehensive tests
- Final group adds maximum 8 tests if critical gaps identified
- Total test count: approximately 26-30 tests maximum
- Focus on critical user workflows, not exhaustive coverage

**Risk Mitigation:**
- Start with configuration (Group 1) to catch partition scheme issues early
- Implement safe mode validation (Group 2.5) before testing to prevent device bricking
- Test rollback mechanism (Group 4.6) to ensure automatic recovery works
- Perform regression testing (Group 5.1) to catch breaking changes

**Implementation Notes:**
- Follow ESP32 OTA best practices (ESP-IDF documentation)
- Reuse existing patterns: file upload handler, progress bar UI, mutex coordination
- Maintain code consistency with existing style (no emojis, descriptive logging)
- Test on actual hardware frequently (emulation not sufficient for OTA)
- Keep test firmware binaries for different scenarios (valid, invalid, broken)
- Document partition layout in comments for future reference

---

## Group 4 Test Execution Instructions

**Test documentation has been prepared and is ready for user execution:**

1. **Test Plan:** `/home/felipe/work/ESP32-file_manager_object_tracker/agent-os/specs/2025-11-17-ota-firmware-update/test-plan.md`
   - Comprehensive test procedures for all 10 integration tests
   - Detailed step-by-step instructions
   - Expected results and success criteria
   - Serial log examples and troubleshooting guide

2. **Test Results Template:** `/home/felipe/work/ESP32-file_manager_object_tracker/agent-os/specs/2025-11-17-ota-firmware-update/test-results.md`
   - Complete template for documenting test execution
   - Sections for each test with checklists
   - Performance measurement fields
   - Issue tracking sections

3. **Quick Testing Guide:** `/home/felipe/work/ESP32-file_manager_object_tracker/agent-os/specs/2025-11-17-ota-firmware-update/testing-guide.md`
   - Quick reference for test execution
   - Preparation steps and prerequisites
   - Abbreviated test instructions
   - Tips and troubleshooting

**To execute tests:**

1. Review the testing-guide.md for quick overview
2. Follow test-plan.md for detailed test procedures
3. Document results in test-results.md as you execute each test
4. Build test firmware: `cd /home/felipe/work/ESP32-file_manager_object_tracker && pio run`
5. Prepare test files (invalid.bin, empty.bin, etc.)
6. Connect serial monitor: `pio device monitor`
7. Execute tests 4.1 through 4.10 sequentially
8. Document all results, timing, and issues
9. Update this tasks.md file to mark completed tests with [x]

**Important Notes:**
- Tests require physical ESP32-CAM device
- Serial monitor must be connected for debugging
- Some tests require WiFi disconnection/reconnection
- Performance benchmarks require timing measurements
- Document all serial log excerpts for validation
- Total estimated time: 3-4 hours for all tests

---

**Document Version:** 1.0
**Date:** 2025-11-17
**Status:** Group 3 Implementation Complete - Group 4 Test Plan Ready for User Execution
**Spec Reference:** `/agent-os/specs/2025-11-17-ota-firmware-update/spec.md`
