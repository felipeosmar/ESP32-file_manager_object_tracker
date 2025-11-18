# OTA Firmware Update - Quick Testing Guide

## Purpose

This guide provides step-by-step instructions for executing the Group 4 Integration & End-to-End Testing for the OTA firmware update feature.

## Before You Start

### What You Need

1. **Hardware:**
   - ESP32-CAM device with OTA firmware installed (Groups 1-3 complete)
   - USB cable for serial monitoring
   - Computer with PlatformIO installed
   - Stable WiFi connection

2. **Software:**
   - Serial monitor (PlatformIO device monitor)
   - Web browser (Chrome, Firefox, or Edge recommended)
   - Text editor for documenting results

3. **Time:**
   - Allow 3-4 hours for complete test execution
   - Tests must be performed sequentially
   - Some tests require device reboots

### Preparation Steps

#### Step 1: Build Test Firmware

```bash
cd /home/felipe/work/ESP32-file_manager_object_tracker
pio run
```

Your firmware will be at: `.pio/build/esp32cam/firmware.bin`

#### Step 2: Create Test Files

```bash
# Create invalid test files
echo "This is not a firmware file" > /tmp/test.txt
cp /tmp/test.txt /tmp/invalid.bin

# Create empty file
touch /tmp/empty.bin

# Find a small image file (or create one)
# Rename it to invalid-image.bin
# Example: cp ~/Pictures/test.jpg /tmp/invalid-image.bin
```

#### Step 3: Connect Serial Monitor

```bash
pio device monitor
```

Keep this terminal window open during all tests.

#### Step 4: Find Device IP Address

Check serial monitor output for a line like:
```
WiFi connected! IP: 192.168.1.XXX
```

Note this IP address. You'll use it to access the device.

#### Step 5: Open Test Documents

Open these files in your text editor:
- `/home/felipe/work/ESP32-file_manager_object_tracker/agent-os/specs/2025-11-17-ota-firmware-update/test-plan.md` (for reference)
- `/home/felipe/work/ESP32-file_manager_object_tracker/agent-os/specs/2025-11-17-ota-firmware-update/test-results.md` (for filling in results)

## Quick Test Execution Checklist

Use this checklist to track your progress through all tests:

- [ ] **Test 4.1:** Complete OTA Upload Flow (~30 min)
- [ ] **Test 4.2:** Drag-and-Drop Upload (~15 min)
- [ ] **Test 4.3:** Binary Validation (~20 min)
- [ ] **Test 4.4:** SD Card Mutex (~15 min)
- [ ] **Test 4.5:** Health Endpoint During OTA (~15 min)
- [ ] **Test 4.6:** Safe Mode Boot Validation (~20 min)
- [ ] **Test 4.7:** Sequential Updates (~30 min)
- [ ] **Test 4.8:** Upload Interruption Recovery (~20 min)
- [ ] **Test 4.9:** Error Handling (~25 min)
- [ ] **Test 4.10:** Performance Benchmarks (~30 min)

## Abbreviated Test Instructions

### Test 4.1: Complete OTA Upload Flow

**What:** Verify end-to-end OTA update works.

**How:**
1. Navigate to `http://192.168.1.XXX/firmware`
2. Select `firmware.bin` file
3. Watch upload progress (0-100%)
4. Monitor status text changes
5. Wait for device reboot
6. Verify auto-reconnect works
7. Check serial logs for validation messages

**Success Criteria:**
- Upload completes without errors
- Device reboots automatically
- Device comes back online
- Safe mode validation succeeds

**Document:** Serial logs, timing, any issues

---

### Test 4.2: Drag-and-Drop Upload

**What:** Verify drag-and-drop works.

**How:**
1. Navigate to firmware page
2. Drag `firmware.bin` over upload zone (don't drop yet)
3. Observe visual feedback
4. Drop file
5. Verify upload proceeds normally

**Success Criteria:**
- Visual feedback on drag-over
- Upload works same as file input

**Document:** UI behavior, any issues

---

### Test 4.3: Binary Validation

**What:** Verify invalid files are rejected.

**How:**
1. Try uploading `invalid.bin` (text file)
2. Verify rejection with error message
3. Try uploading `invalid-image.bin` (JPG file)
4. Verify rejection with error message
5. Try uploading `empty.bin`
6. Verify graceful handling

**Success Criteria:**
- All invalid files rejected
- Clear error messages displayed
- Serial logs show "Invalid firmware: magic byte..."
- Device remains functional

**Document:** Error messages, serial logs

---

### Test 4.4: SD Card Mutex

**What:** Verify SD card operations blocked during OTA.

**How:**
1. Start firmware upload (don't wait for completion)
2. In new tab, go to file manager
3. Try to upload a file
4. Verify 503 error "System busy"
5. Wait for OTA to complete
6. Verify file manager works after OTA

**Success Criteria:**
- File operations blocked during OTA
- Clear "busy" error message
- File operations work after OTA

**Document:** Error response, timing

---

### Test 4.5: Health Endpoint During OTA

**What:** Verify health endpoint works during OTA.

**How:**
1. Start firmware upload
2. In new tab or terminal, query: `curl http://192.168.1.XXX/api/health/status`
3. Repeat at 25%, 50%, 75% progress
4. Check `freeHeap` values

**Success Criteria:**
- Health endpoint responds during OTA
- Free heap > 30KB throughout
- No interference with OTA

**Document:** Free heap values, response times

---

### Test 4.6: Safe Mode Boot Validation

**What:** Verify safe mode prevents rollback.

**How:**
1. Upload firmware
2. Watch serial monitor during reboot
3. Look for "ESP_OTA_IMG_PENDING_VERIFY"
4. Access device homepage within 30 seconds
5. Check serial for "OTA update validated successfully"

**Success Criteria:**
- Partition state starts as PENDING_VERIFY
- First HTTP request triggers validation
- Partition marked valid
- No rollback occurs

**Document:** Serial logs, timing of first request

---

### Test 4.7: Sequential Updates

**What:** Verify multiple updates work.

**How:**
1. Check serial log for current partition (app0 or app1)
2. Upload firmware (Update #1)
3. After reboot, check partition (should switch)
4. Upload firmware again (Update #2)
5. After reboot, check partition (should switch back)

**Success Criteria:**
- Both updates succeed
- Partitions alternate (app0 ↔ app1)
- All features work after each update

**Document:** Partition switching pattern, timing

---

### Test 4.8: Upload Interruption Recovery

**What:** Verify graceful recovery from interrupted upload.

**How:**
1. Start firmware upload
2. At ~40-50% progress, disconnect WiFi
3. Observe browser behavior
4. Reconnect WiFi
5. Verify device still works on old firmware
6. Test file manager operations
7. Try uploading firmware again (should succeed)

**Success Criteria:**
- Interruption handled gracefully
- Device remains functional
- SD card works normally
- Subsequent upload succeeds

**Document:** Serial logs showing abort, recovery behavior

---

### Test 4.9: Error Handling

**What:** Verify all error scenarios show clear messages.

**How:**
1. Test invalid file upload
2. Test upload during OTA (busy state)
3. Test network timeout (if possible)
4. Verify error messages are clear and in Portuguese
5. Verify page remains functional after errors

**Success Criteria:**
- All error scenarios show clear messages
- Consistent error styling
- Page remains usable
- No cryptic technical errors

**Document:** Screenshots of error messages, user experience

---

### Test 4.10: Performance Benchmarks

**What:** Measure performance metrics.

**How:**
1. Note firmware file size
2. Time complete upload process
3. Calculate write speed from serial logs
4. Monitor free heap during upload
5. Time reboot + reconnect
6. Calculate end-to-end time

**Targets:**
- Upload time: < 90 seconds
- Write speed: > 20 KB/s
- Free heap: > 30 KB
- Reboot time: < 15 seconds
- End-to-end: < 120 seconds

**Document:** All measurements, any bottlenecks

---

## Tips for Effective Testing

### Serial Monitor Tips

1. **Keep it running:** Don't close serial monitor between tests
2. **Copy important logs:** Highlight and copy key serial output
3. **Note timestamps:** Serial logs include timestamps for debugging
4. **Watch for errors:** Red text or "ERROR:" messages are important

### Browser Tips

1. **Use multiple tabs:** One for testing, one for documentation
2. **Developer tools:** Open browser DevTools (F12) to see network traffic
3. **Clear cache:** If page behaves strangely, clear browser cache
4. **Check console:** Browser console shows JavaScript errors

### Common Issues and Solutions

**Issue: Device IP changes after reboot**
- Solution: Check serial monitor for new IP, update your browser URL

**Issue: Upload seems stuck**
- Solution: Check WiFi signal, check serial monitor for errors

**Issue: Auto-reconnect fails**
- Solution: Manually navigate to device IP after reboot

**Issue: Serial monitor shows errors**
- Solution: Document the error, continue with tests, note in results

**Issue: Can't access device after OTA**
- Solution: Wait 60 seconds for automatic rollback, or reflash via USB

## Documenting Results

### What to Document

For each test, document:

1. **Status:** PASS / FAIL / BLOCKED / NOT RUN
2. **Execution time:** How long the test took
3. **Actual results:** What actually happened
4. **Issues found:** Any problems or unexpected behavior
5. **Serial logs:** Relevant excerpts from serial monitor
6. **Observations:** Notes about user experience, performance, etc.

### How to Document

1. **Use the test-results.md file:** Fill in the template as you go
2. **Copy serial logs:** Paste relevant serial output into the document
3. **Take screenshots:** Capture error messages, UI states
4. **Be specific:** Include exact error messages, timing measurements
5. **Note the unexpected:** Document anything that surprises you

### Critical Information to Capture

- Device IP address
- Firmware file size
- Upload timing measurements
- Free heap values
- Partition switching pattern
- Error messages (exact text)
- Serial log excerpts for key events

## After Testing

### Immediate Actions

1. **Review all test results:** Check that all 10 tests are documented
2. **Identify critical issues:** Mark any blocking problems
3. **Calculate pass rate:** Count passed vs. failed tests
4. **Complete summary section:** Fill in test results summary table

### Next Steps

1. **Report results:** Share test-results.md with team
2. **File issues:** Create issue tickets for any bugs found
3. **Update tasks.md:** Mark Group 4 tasks as complete
4. **Prepare for Group 5:** Ready for regression testing if all tests pass

### Success Criteria for Group 4 Completion

Group 4 is complete when:

- [ ] All 10 tests have been executed
- [ ] Test results are documented in test-results.md
- [ ] Critical issues are identified and documented
- [ ] Performance benchmarks are measured and recorded
- [ ] Pass/fail status determined for each test
- [ ] Serial logs captured for key scenarios
- [ ] Overall assessment completed (GO / NO-GO decision)

## Questions or Issues?

If you encounter problems during testing:

1. **Check serial monitor first:** Most issues show up there
2. **Review test plan:** Detailed instructions in test-plan.md
3. **Check device accessibility:** Ping device IP, check WiFi
4. **Try device reboot:** Power cycle can resolve many issues
5. **Document the problem:** Even if you can't solve it, document it

## Test Execution Log

Keep a simple log of when you execute each test:

```
Test 4.1: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.2: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.3: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.4: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.5: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.6: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.7: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.8: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.9: Started [date/time], Completed [date/time], Status: [PASS/FAIL]
Test 4.10: Started [date/time], Completed [date/time], Status: [PASS/FAIL]

Total time: _____ hours
Overall result: _____ tests passed, _____ tests failed
```

---

Good luck with testing! Remember to be thorough but also practical - the goal is to verify the OTA feature works reliably and safely.
