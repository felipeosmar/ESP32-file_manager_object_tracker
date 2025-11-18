# OTA Firmware Update - Integration Test Plan

## Document Information
- **Version:** 1.0
- **Date:** 2025-11-18
- **Test Group:** Group 4 - Integration & End-to-End Testing
- **Dependencies:** Groups 1, 2, 3 (Configuration, Backend, Frontend) - COMPLETE
- **Estimated Testing Time:** 3-4 hours

## Overview

This test plan covers comprehensive integration and end-to-end testing for the OTA (Over-The-Air) firmware update feature. All tests require physical access to the ESP32-CAM device and should be executed manually by the user.

**Test Environment Requirements:**
- ESP32-CAM device with OTA feature implemented (Groups 1-3 complete)
- Computer with PlatformIO installed for building test firmware
- WiFi network connection
- Web browser for accessing device interface
- Serial monitor access for debugging output
- Test firmware binaries (.bin files)

## Pre-Test Preparation

### Build Test Firmware
1. Ensure current firmware is built and ready:
   ```bash
   cd /home/felipe/work/ESP32-file_manager_object_tracker
   pio run
   ```

2. Locate firmware binary:
   ```
   .pio/build/esp32cam/firmware.bin
   ```

3. Create test file set:
   - Valid firmware: `firmware.bin` (from PlatformIO build)
   - Invalid text file: Create `test.txt` with random text, rename to `invalid.bin`
   - Invalid image file: Use any `.jpg` file, rename to `invalid-image.bin`
   - Empty file: `touch empty.bin` to create empty binary

4. Note device IP address:
   - Check serial monitor output after boot
   - Or check router DHCP leases
   - Format: `http://192.168.1.XXX`

### Serial Monitor Setup
1. Connect device to computer via USB
2. Open serial monitor:
   ```bash
   pio device monitor
   ```
3. Keep serial monitor open during all tests for debugging

## Test Suite

---

### Test 4.1: Complete OTA Upload Flow (End-to-End)

**Objective:** Verify the complete firmware update workflow from file selection through device reboot and validation.

**Prerequisites:**
- Device running current firmware
- Valid `firmware.bin` file available
- Serial monitor connected

**Test Steps:**

1. **Access Firmware Page**
   - Open browser to device IP: `http://192.168.1.XXX/firmware`
   - Verify page loads with correct layout
   - Check navigation buttons are present (Home, Health, Firmware, Files)

2. **Select Firmware File**
   - Click on upload zone or file input
   - Select `firmware.bin` file from filesystem
   - Verify file name displays in UI

3. **Monitor Upload Progress**
   - Observe progress bar starts at 0%
   - Watch progress bar animate to 100%
   - Verify percentage text updates continuously
   - Note upload time (target: < 60 seconds for 1-2MB file)

4. **Monitor Status Text Updates**
   - Verify status shows: "Enviando firmware..." (0-95%)
   - Verify status shows: "Validando arquivo..." (95-96%)
   - Verify status shows: "Gravando na flash..." (96-99%)
   - Verify status shows: "Reiniciando dispositivo..." (100%)

5. **Monitor Serial Output During Upload**
   - Check for: "OTA Update started: firmware.bin"
   - Check for: "Firmware validation passed: ESP32 magic byte detected"
   - Check for: "OTA Update initialized successfully"
   - Check for: "OTA Progress: X bytes written" (every 64KB)
   - Check for: "OTA Update completed successfully: X bytes"
   - Check for: "OTA Update successful! Rebooting..."

6. **Wait for Device Reboot**
   - Observe device restart in serial monitor
   - Wait approximately 10 seconds
   - Monitor serial output for boot messages

7. **Verify Auto-Reconnect**
   - Watch browser page attempt reconnection
   - Verify polling messages (every 3 seconds)
   - Confirm successful reconnection message displays
   - Verify page automatically reloads to homepage

8. **Verify Device Functionality**
   - Check homepage loads correctly
   - Access `/health` page - verify health metrics display
   - Access `/filemanager` page - verify file operations work
   - Check serial monitor for: "First boot after OTA update detected"
   - Check serial monitor for: "OTA update validated successfully - rollback cancelled"

**Expected Results:**
- [  ] Upload progress bar animates smoothly 0-100%
- [  ] Status text updates through all stages
- [  ] Serial logs show complete upload sequence
- [  ] Device reboots within 2-3 seconds of completion
- [  ] Auto-reconnect succeeds within 60 seconds
- [  ] Device loads homepage successfully
- [  ] Safe mode validation marks partition valid
- [  ] No rollback occurs

**Actual Results:**
```
[User will fill in test execution results]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User will add observations, timing measurements, issues]
```

---

### Test 4.2: Drag-and-Drop Upload

**Objective:** Verify firmware upload works via drag-and-drop interface.

**Prerequisites:**
- Device running and accessible
- Valid `firmware.bin` file available
- Browser with drag-and-drop support

**Test Steps:**

1. **Access Firmware Page**
   - Navigate to `http://192.168.1.XXX/firmware`

2. **Test Drag-Over Visual Feedback**
   - Drag `firmware.bin` file over upload zone
   - **DO NOT DROP YET**
   - Verify border color changes (highlight effect)
   - Verify background color changes
   - Verify "dragging" visual state is active

3. **Test Drag-Leave Feedback**
   - Drag file away from upload zone
   - Verify visual feedback returns to normal
   - Verify no upload initiated

4. **Perform Drop Upload**
   - Drag `firmware.bin` over upload zone again
   - Release mouse button (drop file)
   - Verify upload begins immediately

5. **Monitor Upload Process**
   - Verify progress bar displays and animates
   - Verify status text updates
   - Verify upload completes successfully
   - Verify device reboots
   - Verify auto-reconnect works

**Expected Results:**
- [  ] Drag-over shows visual feedback (border/background change)
- [  ] Drag-leave removes visual feedback
- [  ] Drop initiates upload immediately
- [  ] Upload flow identical to file input method
- [  ] Complete workflow succeeds

**Actual Results:**
```
[User will fill in test execution results]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User will add observations]
```

---

### Test 4.3: Binary Validation with Invalid Files

**Objective:** Verify the system rejects invalid files and displays appropriate error messages.

**Prerequisites:**
- Device running and accessible
- Test file set prepared (invalid.bin, invalid-image.bin, empty.bin)

**Test Steps:**

**Subtest A: Text File Renamed as .bin**

1. Access firmware page
2. Select `invalid.bin` (renamed text file)
3. Observe upload behavior
4. Check serial monitor for validation message
5. Verify error message displays in UI
6. Expected serial log: "Invalid firmware: magic byte is 0xXX, expected 0xE9"
7. Expected UI: Red error banner with message

**Subtest B: Image File Renamed as .bin**

1. Refresh firmware page (clear any previous errors)
2. Select `invalid-image.bin` (renamed JPG file)
3. Observe upload behavior
4. Check serial monitor for validation message
5. Verify error message displays in UI
6. JPG magic bytes are 0xFF 0xD8, should fail validation

**Subtest C: Empty File**

1. Refresh firmware page
2. Select `empty.bin` (0 bytes)
3. Observe upload behavior
4. Check serial monitor for validation message
5. Verify graceful failure with error message

**Expected Results:**
- [  ] Text file rejected with magic byte mismatch error
- [  ] Image file rejected with magic byte mismatch error
- [  ] Empty file rejected gracefully
- [  ] Serial logs show validation failures
- [  ] UI displays clear error messages in all cases
- [  ] Device remains functional after rejection
- [  ] No flash write operation attempted for invalid files

**Actual Results:**
```
Subtest A (Text File):
[User results]

Subtest B (Image File):
[User results]

Subtest C (Empty File):
[User results]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations]
```

---

### Test 4.4: SD Card Mutex Coordination

**Objective:** Verify SD card operations are blocked during OTA upload to prevent corruption.

**Prerequisites:**
- Device running and accessible
- Large firmware file (or slow network) to extend upload time
- Test file for file manager upload

**Test Steps:**

1. **Initiate OTA Upload**
   - Access firmware page
   - Start uploading `firmware.bin`
   - **DO NOT WAIT FOR COMPLETION**

2. **Attempt File Manager Upload During OTA**
   - Open new browser tab
   - Navigate to `http://192.168.1.XXX/filemanager`
   - Try to upload any file via file manager
   - Observe response

3. **Verify Busy Error**
   - Check for 503 error response
   - Verify error message: "System busy - firmware update in progress"
   - Confirm upload is rejected

4. **Monitor Serial Output**
   - Check serial logs for mutex acquisition message
   - Verify OTA upload continues uninterrupted

5. **Wait for OTA Completion**
   - Let firmware upload complete
   - Let device reboot
   - Wait for device to come back online

6. **Verify File Manager Works After OTA**
   - Access file manager page
   - Upload a test file
   - Verify upload succeeds
   - Confirm SD card operations work normally

**Expected Results:**
- [  ] File upload fails with 503 error during OTA
- [  ] Error message clearly states "firmware update in progress"
- [  ] OTA upload completes successfully despite concurrent attempt
- [  ] File manager operations work normally after OTA completes
- [  ] No SD card corruption occurs
- [  ] Serial logs show mutex coordination

**Actual Results:**
```
[User will fill in test execution results]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations, timing notes]
```

---

### Test 4.5: Health Endpoint During OTA

**Objective:** Verify non-SD endpoints remain accessible during firmware upload.

**Prerequisites:**
- Device running and accessible
- Valid firmware file

**Test Steps:**

1. **Start Firmware Upload**
   - Access firmware page
   - Initiate `firmware.bin` upload
   - **DO NOT WAIT FOR COMPLETION**

2. **Query Health Endpoint**
   - Open new browser tab or use curl:
     ```bash
     curl http://192.168.1.XXX/api/health/status
     ```
   - Observe response time
   - Verify JSON response received

3. **Check Response Content**
   - Verify response includes:
     - `freeHeap`
     - `heapSize`
     - `uptimeSeconds`
     - `wifiSignal`
   - Note free heap value (should be > 30KB during upload)

4. **Monitor During Upload Progress**
   - Query health endpoint multiple times during upload
   - At 25% progress
   - At 50% progress
   - At 75% progress
   - Verify all requests succeed

**Expected Results:**
- [  ] Health endpoint responds successfully during OTA
- [  ] Response time < 2 seconds even during upload
- [  ] Free heap remains > 30KB throughout upload
- [  ] All queries succeed (no timeouts or errors)
- [  ] JSON structure is valid
- [  ] No interference with OTA upload process

**Actual Results:**
```
Initial free heap: ___ KB
At 25% progress: ___ KB
At 50% progress: ___ KB
At 75% progress: ___ KB

Response times:
[User will record timing data]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations]
```

---

### Test 4.6: Safe Mode Boot Validation

**Objective:** Verify safe mode mechanism prevents rollback when device boots successfully.

**Prerequisites:**
- Device running and accessible
- Valid firmware file
- Serial monitor connected

**Test Steps:**

1. **Upload Firmware**
   - Access firmware page
   - Upload valid `firmware.bin`
   - Wait for upload to complete
   - Observe device reboot notification

2. **Monitor Serial Output During Boot**
   - Watch serial monitor for boot sequence
   - Look for partition state message
   - **CRITICAL:** Note the partition state immediately after boot
   - Expected: "Partition state: ESP_OTA_IMG_PENDING_VERIFY"

3. **Access Device Within 30 Seconds**
   - Immediately navigate to homepage: `http://192.168.1.XXX/`
   - Or any page (health, firmware, filemanager)
   - **IMPORTANT:** Must access within 30 seconds of boot

4. **Monitor Serial Validation Messages**
   - Check for: "First boot after OTA update detected"
   - Check for: "Web server responding successfully - marking partition valid"
   - Check for: "OTA update validated successfully - rollback cancelled"

5. **Verify No Rollback Occurs**
   - Device remains running new firmware
   - No unexpected reboots
   - Web interface continues to work

6. **Check Partition State**
   - Monitor serial logs
   - Should show partition state changes to: ESP_OTA_IMG_VALID

**Expected Results:**
- [  ] Boot shows partition state: ESP_OTA_IMG_PENDING_VERIFY
- [  ] First HTTP request triggers validation
- [  ] Serial logs show "marking partition valid" message
- [  ] Serial logs show "rollback cancelled" message
- [  ] No unexpected reboots occur
- [  ] Device continues running new firmware
- [  ] Partition state changes to ESP_OTA_IMG_VALID

**Actual Results:**
```
Partition state at boot: ___________
Time of first HTTP request: ___ seconds after boot
Validation messages:
[User will paste serial log excerpt]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations about timing, any issues]
```

---

### Test 4.7: Sequential Firmware Updates

**Objective:** Verify multiple firmware updates work in sequence, testing both app0→app1 and app1→app0 transitions.

**Prerequisites:**
- Device running and accessible
- Two different firmware binaries (can be same firmware, just need to track which partition is active)

**Test Steps:**

1. **Check Current Partition**
   - Monitor serial output at boot
   - Note which partition is active (app0 or app1)
   - Example log: "Running from partition: app0" or "app1"

2. **First Update (A → B)**
   - Access firmware page
   - Upload firmware (call this "Version A")
   - Monitor upload and reboot process
   - Verify device comes back online
   - Check serial log for partition switch
   - Note new partition (should be different from step 1)

3. **Verify Device Functionality After First Update**
   - Test homepage loads
   - Test health page works
   - Test file manager works
   - Confirm all features operational

4. **Second Update (B → C)**
   - Access firmware page again
   - Upload firmware (call this "Version B")
   - Monitor upload and reboot process
   - Verify device comes back online
   - Check serial log for partition switch
   - Note partition (should switch back to original)

5. **Verify Device Functionality After Second Update**
   - Test homepage loads
   - Test health page works
   - Test file manager works
   - Confirm all features operational

6. **Verify Partition Switching Pattern**
   - Confirm updates alternate between partitions
   - Example: Start app0 → update to app1 → update to app0

**Expected Results:**
- [  ] First update completes successfully
- [  ] Device boots from new partition after first update
- [  ] Second update completes successfully
- [  ] Device boots from alternate partition after second update
- [  ] Partition switching follows expected pattern (app0 ↔ app1)
- [  ] All features work after each update
- [  ] No degradation in performance or functionality
- [  ] Both update directions work (app0→app1 and app1→app0)

**Actual Results:**
```
Initial partition: app___
After first update: app___
After second update: app___

Update timings:
First update total time: ___ seconds
Second update total time: ___ seconds

[User will add observations]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations]
```

---

### Test 4.8: Upload Interruption Recovery

**Objective:** Verify device recovers gracefully from interrupted OTA upload without corruption.

**Prerequisites:**
- Device running and accessible
- Valid firmware file
- Ability to disconnect WiFi (router control or disable WiFi on device)

**Test Steps:**

1. **Start Firmware Upload**
   - Access firmware page
   - Begin uploading `firmware.bin`
   - Monitor progress bar
   - Wait until ~40-50% progress

2. **Interrupt Upload**
   - Disconnect WiFi (turn off router or disconnect device from network)
   - Observe browser behavior
   - Note any error messages
   - Monitor serial output for error handling

3. **Reconnect WiFi**
   - Restore WiFi connection
   - Wait for device to reconnect to network
   - Check serial monitor for WiFi reconnection

4. **Verify Device Functionality**
   - Navigate to device homepage
   - Confirm page loads correctly
   - Verify device still running OLD firmware (update didn't apply)

5. **Test SD Card Operations**
   - Access file manager
   - Try uploading a test file
   - Try downloading a file
   - Try deleting a file
   - Verify all operations work normally

6. **Verify No Corruption**
   - Check serial logs for any error messages
   - Confirm no watchdog resets occurred
   - Verify device stable and responsive

7. **Retry Firmware Upload**
   - Access firmware page again
   - Upload firmware completely (without interruption)
   - Verify upload succeeds this time
   - Confirm device can still perform successful OTA update

**Expected Results:**
- [  ] Upload interruption detected and handled gracefully
- [  ] Browser shows appropriate error message
- [  ] Serial logs show Update.abort() called
- [  ] Device remains running old firmware
- [  ] SD card operations work normally after interruption
- [  ] No file system corruption
- [  ] No unexpected reboots or crashes
- [  ] Subsequent OTA upload succeeds
- [  ] Device recovers to fully functional state

**Actual Results:**
```
Progress when interrupted: ___%
Error message displayed: ___________
Serial log excerpt:
[User will paste relevant serial output]

File manager test results:
[User will document SD card operation tests]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations]
```

---

### Test 4.9: Error Handling and User Feedback

**Objective:** Verify all error scenarios display clear, helpful messages to users.

**Prerequisites:**
- Device running and accessible
- Various test files prepared

**Test Steps:**

**Scenario A: Invalid File Type**

1. Access firmware page
2. Try to select a `.txt` file (client-side validation should prevent)
3. If client allows, observe server response
4. Verify clear error message displays

**Scenario B: Invalid Binary Format**

1. Access firmware page
2. Upload `invalid.bin` (non-ESP32 binary)
3. Observe error message
4. Verify message explains the issue clearly
5. Check serial logs for detailed error

**Scenario C: System Busy (OTA in Progress)**

1. Start firmware upload in one browser tab
2. Open firmware page in another tab
3. Try to start second upload
4. Verify error message indicates system busy

**Scenario D: Network Timeout**

1. Start firmware upload
2. Pause upload mid-way (browser developer tools network throttling)
3. Let upload timeout
4. Verify timeout error message displays

**Scenario E: SD Card Busy**

1. Start large file upload to file manager
2. Try to access firmware page
3. Verify any SD card access issues are handled

**Test Each Error Scenario For:**
- Error message is displayed to user
- Message is in Portuguese (matching app language)
- Message is clear and actionable
- Error banner uses appropriate color (red for critical, orange for warning)
- Error can be dismissed (if applicable)
- Page remains functional after error
- User can retry the operation

**Expected Results:**
- [  ] Invalid file type: Clear rejection message
- [  ] Invalid binary: "Arquivo inválido: não é um firmware ESP32"
- [  ] System busy: "Sistema ocupado - atualização em andamento"
- [  ] Network timeout: Timeout error with retry option
- [  ] All errors use consistent styling
- [  ] All error messages are user-friendly
- [  ] Page remains usable after errors
- [  ] No cryptic technical jargon in user-facing messages

**Actual Results:**
```
Scenario A:
[User results]

Scenario B:
[User results]

Scenario C:
[User results]

Scenario D:
[User results]

Scenario E:
[User results]
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations]
```

---

### Test 4.10: Performance and Timing Benchmarks

**Objective:** Measure and verify performance metrics meet target specifications.

**Prerequisites:**
- Device running and accessible
- Firmware file of known size (check .pio/build/esp32cam/firmware.bin size)
- Stopwatch or timer
- Serial monitor for detailed timing

**Test Steps:**

**Benchmark A: 1MB Firmware Upload Time**

1. Prepare firmware file
   - Check file size: `ls -lh .pio/build/esp32cam/firmware.bin`
   - Note exact size
2. Access firmware page
3. Start timer when upload begins
4. Monitor progress bar
5. Stop timer when device begins reboot
6. Calculate total time:
   - Upload time (network transfer)
   - Flash write time (visible in serial logs)
   - Total time until reboot
7. **Target: < 90 seconds total**

**Benchmark B: Flash Write Speed**

1. Monitor serial output during upload
2. Note progress messages: "OTA Progress: X bytes written"
3. Calculate write speed from serial logs
4. Formula: (Total bytes) / (Time in seconds) = Bytes/sec
5. Convert to KB/s
6. **Target: > 20 KB/s write speed**

**Benchmark C: Memory Usage During Upload**

1. Before starting upload, query health endpoint:
   ```bash
   curl http://192.168.1.XXX/api/health/status
   ```
2. Note initial `freeHeap` value
3. Start firmware upload
4. During upload, query health endpoint multiple times:
   - At 25% progress
   - At 50% progress
   - At 75% progress
   - At 95% progress
5. Record minimum free heap observed
6. **Target: Free heap > 30KB during upload**

**Benchmark D: Reboot and Reconnection Time**

1. Upload firmware
2. Start timer when "Rebooting..." message appears
3. Monitor auto-reconnect polling
4. Stop timer when homepage successfully loads
5. **Target: < 15 seconds reboot + reconnect**

**Benchmark E: End-to-End Total Time**

1. Start timer when clicking "upload"
2. Stop timer when homepage reloads after OTA
3. Include all phases:
   - File selection
   - Upload
   - Validation
   - Flash write
   - Reboot
   - Reconnection
4. **Target: < 120 seconds (2 minutes) total**

**Expected Results:**
- [  ] Upload time < 90 seconds for 1-2MB firmware
- [  ] Flash write speed > 20 KB/s
- [  ] Free heap > 30KB during upload
- [  ] Reboot + reconnect < 15 seconds
- [  ] End-to-end time < 120 seconds
- [  ] Performance consistent across multiple uploads
- [  ] No memory leaks detected

**Actual Results:**
```
Firmware Size: ___ bytes (___ MB)

Benchmark A - Upload Time:
Network transfer: ___ seconds
Flash write: ___ seconds
Total to reboot: ___ seconds
Status: PASS / FAIL (target: < 90s)

Benchmark B - Write Speed:
Average write speed: ___ KB/s
Peak write speed: ___ KB/s
Status: PASS / FAIL (target: > 20 KB/s)

Benchmark C - Memory Usage:
Initial free heap: ___ KB
At 25% progress: ___ KB
At 50% progress: ___ KB
At 75% progress: ___ KB
At 95% progress: ___ KB
Minimum free heap: ___ KB
Status: PASS / FAIL (target: > 30 KB)

Benchmark D - Reboot Time:
Reboot + reconnect: ___ seconds
Status: PASS / FAIL (target: < 15s)

Benchmark E - End-to-End:
Total time: ___ seconds
Status: PASS / FAIL (target: < 120s)
```

**Status:** [ ] PASS / [ ] FAIL

**Notes:**
```
[User observations about performance, any bottlenecks noticed]
```

---

## Test Execution Summary

### Overall Test Results

| Test ID | Test Name | Status | Duration | Notes |
|---------|-----------|--------|----------|-------|
| 4.1 | Complete OTA Upload Flow | [ ] PASS / [ ] FAIL | ___ min | |
| 4.2 | Drag-and-Drop Upload | [ ] PASS / [ ] FAIL | ___ min | |
| 4.3 | Binary Validation | [ ] PASS / [ ] FAIL | ___ min | |
| 4.4 | SD Card Mutex | [ ] PASS / [ ] FAIL | ___ min | |
| 4.5 | Health Endpoint During OTA | [ ] PASS / [ ] FAIL | ___ min | |
| 4.6 | Safe Mode Boot Validation | [ ] PASS / [ ] FAIL | ___ min | |
| 4.7 | Sequential Updates | [ ] PASS / [ ] FAIL | ___ min | |
| 4.8 | Upload Interruption Recovery | [ ] PASS / [ ] FAIL | ___ min | |
| 4.9 | Error Handling | [ ] PASS / [ ] FAIL | ___ min | |
| 4.10 | Performance Benchmarks | [ ] PASS / [ ] FAIL | ___ min | |

**Total Tests:** 10
**Passed:** ___
**Failed:** ___
**Blocked:** ___
**Not Run:** ___

### Critical Issues Found
```
[User will document any critical issues that block functionality]
```

### Non-Critical Issues Found
```
[User will document minor issues, UI glitches, performance concerns]
```

### Performance Summary
```
Average upload time: ___ seconds
Average write speed: ___ KB/s
Minimum free heap: ___ KB
Average reboot time: ___ seconds
```

### Test Environment Details
```
Device Model: ESP32-CAM
Firmware Version: ___________
PlatformIO Version: ___________
Browser: ___________
WiFi Signal Strength: ___ dBm
Test Date: ___________
Tester Name: ___________
```

## Recommendations

### For Production Deployment
- [ ] Document successful test completion
- [ ] Create backup of working firmware
- [ ] Test on multiple devices if deploying to fleet
- [ ] Consider adding authentication before production use
- [ ] Monitor initial deployments closely

### For Future Improvements
- [ ] Consider adding upload resume capability
- [ ] Implement firmware version display
- [ ] Add firmware size validation before upload
- [ ] Consider compression for faster uploads
- [ ] Add option to schedule updates

## Appendix

### Serial Log Examples

**Successful OTA Upload:**
```
[User should paste example of successful OTA serial log]
```

**Failed Upload (Invalid Binary):**
```
[User should paste example of validation failure log]
```

**Safe Mode Validation:**
```
[User should paste example of boot validation log]
```

### Troubleshooting Guide

**Issue: Upload Stalls at X%**
- Check WiFi signal strength
- Verify SD card mutex is released
- Check free heap memory
- Review serial logs for errors

**Issue: Device Won't Boot After OTA**
- Wait 60 seconds for automatic rollback
- Device should revert to previous firmware
- Check serial logs for rollback messages
- If stuck, reflash via USB

**Issue: Validation Fails on Valid Firmware**
- Verify firmware.bin is from correct PlatformIO build
- Check file wasn't corrupted during transfer
- Rebuild firmware and try again

**Issue: Auto-Reconnect Fails**
- Check device IP didn't change (DHCP)
- Verify WiFi credentials in config
- Manually navigate to device IP
- Check router for device connection

---

**End of Test Plan**
