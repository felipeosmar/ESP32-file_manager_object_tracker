# Bug Fix: SD Card Initialization Failure After OTA Update

## Problem Description

After successfully uploading firmware via OTA, the ESP32-CAM would reboot but fail to initialize the SD card with the following error:

```
E (559) sdmmc_common: sdmmc_init_ocr: send_op_cond (1) returned 0x107
E (560) vfs_fat_sdmmc: sdmmc_card_init failed (0x107).
SD Card Mount Failed
SD Card initialization failed!
```

## Root Cause Analysis

### Observed Symptoms

During OTA upload, the serial logs showed camera-related errors:
```
cam_hal: EV-EOF-OVF
cam_hal: EV-VSYNC-OVF
```

These messages indicate that the **camera was still active during the OTA flash process**.

### Technical Explanation

The ESP32-CAM has a hardware limitation: **the camera and SD card share some GPIO pins**:

- **Camera pins**: GPIO 4, 5, 18, 19, 21, 22, 23, 25, 26, 27
- **SD card pins (SDMMC)**: GPIO 2, 4, 12, 13, 14, 15

Specifically, **GPIO 4** is shared between:
- Camera: HREF (Horizontal Reference) signal
- SD card: Data line D1

When the camera continues running during OTA:
1. Camera keeps generating interrupts and using the shared GPIO pins
2. The flash write process interferes with the camera operation
3. After reboot, the GPIO pins may be in an incorrect state
4. SD card initialization fails because the pins are not properly configured

## Solution Implemented

Added **camera pause and deinitialization** before starting the OTA update process to prevent spinlock conflicts.

### Code Changes

**File:** `src/main.cpp`

**1. Added camera control flag (Line 45):**
```cpp
bool cameraActive = true; // Flag to control camera access during OTA
```

**2. Pause camera access in loop() (Lines 135-139):**
```cpp
// Skip camera operations if OTA upload is in progress
if (!cameraActive) {
  delay(100);
  return;
}
```

**3. Pause camera streaming (Lines 1059-1063):**
```cpp
// Check if camera is active (not during OTA)
if (!cameraActive) {
  delay(100);
  return 0; // Stop streaming during OTA
}
```

**4. Stop camera before OTA (Lines 569-582 in OTA upload handler):**
```cpp
// Stop camera access from loop() task
cameraActive = false;
Serial.println("Camera access paused");
delay(200); // Give time for loop() to exit camera operations

// Deinitialize camera to free shared pins (SD card shares pins with camera)
Serial.println("Deinitializing camera before OTA...");
esp_err_t err = esp_camera_deinit();
if (err != ESP_OK) {
  Serial.printf("Camera deinit warning: %d\n", err);
} else {
  Serial.println("Camera deinitialized successfully");
}
delay(100); // Give time for camera to fully stop
```

### Why This Works

1. **Stops concurrent access**: `cameraActive = false` prevents loop() and streaming from accessing camera
2. **Waits for tasks to exit**: 200ms delay ensures any ongoing `esp_camera_fb_get()` completes
3. **Prevents spinlock conflicts**: No camera operations compete for resources during deinit
4. **Releases shared GPIO pins**: `esp_camera_deinit()` stops the camera and releases all GPIO pins
5. **Prevents interrupt conflicts**: No more camera interrupts during OTA flash write
6. **Clean GPIO state after reboot**: Pins are properly released before restart
7. **Allows SD card to reinitialize**: After reboot, SD card gets clean access to the shared pins

## Testing

### Before Fix
- OTA upload: ✓ Success
- Device reboot: ✓ Success
- SD card initialization: ✗ **FAILED** (error 0x107)
- Camera interrupts during OTA: ✗ Present (`cam_hal: EV-VSYNC-OVF`)

### After Fix (Expected)
- OTA upload: ✓ Success
- Camera deinitialization: ✓ Success
- Device reboot: ✓ Success
- SD card initialization: ✓ Success
- Camera interrupts during OTA: ✓ None (camera stopped)

### Serial Log Verification

After applying the fix, you should see:
```
OTA Update started: firmware.bin
Camera access paused
Deinitializing camera before OTA...
Camera deinitialized successfully
OTA upload - SD card mutex acquired
Firmware validation passed: ESP32 magic byte detected
OTA Update initialized successfully
OTA Progress: XXXXX bytes written
[NO camera interrupts here]
[NO spinlock assert errors]
OTA Update completed successfully: XXXXX bytes
OTA Update successful! Rebooting...

[After reboot]
Initializing SD card...
SD Card Type: SDSC
SD Card Size: 1932MB
SD Card initialized successfully  <-- SUCCESS!
```

## Additional Notes

### Camera Reinitialize

The camera will automatically reinitialize on the next boot during the normal `setup()` function. No changes needed to the initialization code.

### Impact on OTA Upload

- **Performance**: No impact. Camera was not used during OTA anyway.
- **Duration**: Adds ~100ms for camera deinit, negligible compared to flash write time
- **Safety**: Improves reliability by preventing GPIO conflicts

### ESP32-CAM Pin Sharing Reference

For future reference, here are the shared resources on ESP32-CAM:

| GPIO | Camera Function | SD Card Function | Conflict Risk |
|------|----------------|------------------|---------------|
| 4    | HREF           | DATA1            | **HIGH**      |
| 12   | -              | DATA2            | Low           |
| 13   | -              | DATA3            | Low           |
| 14   | -              | CLK              | Low           |
| 15   | -              | CMD              | Low           |
| 2    | -              | DATA0            | Low           |

GPIO 4 has the highest conflict risk because it's actively used by the camera HREF signal.

## Related Issues

This fix also prevents:
- Potential flash corruption during OTA (camera DMA conflicts)
- Unstable GPIO states after reset
- Inconsistent SD card behavior between boots

## Commit Information

**Date:** 2025-11-18
**Affected Files:** `src/main.cpp`
**Build Status:** ✓ Success (1,018,461 bytes, 51.8% flash usage)
**Testing Status:** Ready for verification

## Verification Steps

To verify this fix works:

1. Build and flash the updated firmware:
   ```bash
   pio run --target upload
   ```

2. Monitor serial output:
   ```bash
   pio device monitor
   ```

3. Perform OTA update via web interface at `http://[ESP32-IP]/firmware`

4. Verify in serial logs:
   - "Deinitializing camera before OTA..." appears
   - "Camera deinitialized successfully" appears
   - NO `cam_hal: EV-VSYNC-OVF` errors during OTA
   - After reboot: "SD Card initialized successfully" appears

5. Confirm web interface loads correctly after OTA

## References

- ESP32 Camera Driver: `esp_camera.h` - `esp_camera_deinit()`
- ESP32 SD_MMC Library: Shared pin configuration
- ESP-IDF SDMMC Error Codes: 0x107 = ESP_ERR_TIMEOUT
