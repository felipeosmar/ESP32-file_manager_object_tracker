# Product Roadmap

1. [ ] **TensorFlow Lite Model Integration** — Add support for loading and running TensorFlow Lite models from SD card, starting with a pre-trained object detection model (MobileNet SSD or similar). Implement model inference in the tracking loop, display detection results on web interface with bounding boxes, and create model swapping mechanism without firmware recompilation. `L`

2. [ ] **ML Model Management Interface** — Create web interface for uploading, selecting, and configuring TensorFlow Lite models directly from browser. Include model metadata display (input size, classes, confidence thresholds), test inference interface with live camera feed, and model performance metrics (inference time, memory usage). `M`

3. [ ] **Video Recording & Snapshot Capture** — Implement motion-triggered video recording to SD card in MJPEG format with configurable duration and quality settings. Add manual snapshot capture button to web interface, create gallery view for browsing recorded clips and images, and implement file size management with automatic cleanup of old recordings. `L`

4. [ ] **Object Classification & Labeling** — Extend TensorFlow Lite integration to classify detected objects and display labels on web interface. Implement confidence threshold filtering, multi-object detection support with individual tracking for highest-confidence target, and detection event logging to JSON file on SD card. `M`

5. [ ] **MQTT Publisher Integration** — Add MQTT client capability to publish detection events, camera snapshots (as base64 or URLs), and system status to configurable MQTT broker. Implement topic configuration via JSON, retained message support for status updates, and Last Will and Testament for connection monitoring. `M`

6. [ ] **Home Assistant Discovery & Integration** — Implement Home Assistant MQTT discovery protocol to automatically create camera, binary sensor (motion detected), and sensor entities (tracking status, system health). Add configurable device information, state updates on detection events, and camera snapshot service integration. `M`

7. [ ] **Multi-Object Tracking** — Enhance tracking system to detect and track multiple objects simultaneously with visual indicators for each tracked object on web interface. Implement priority-based tracking (follow largest, closest, or highest-confidence object), track history visualization, and object persistence across frames to reduce flickering. `L`

8. [ ] **Advanced Detection Zones** — Create web-based interface for drawing detection zones (inclusion areas) and exclusion zones directly on camera preview. Implement zone-based tracking that only responds to motion within defined areas, zone crossing events for automation triggers, and per-zone sensitivity settings. `M`

9. [ ] **Authentication & Access Control** — Add HTTP basic authentication to web interface with configurable username/password stored in config.json. Implement session management, separate read-only and admin access levels, and API token support for programmatic access without exposing credentials. `S`

10. [ ] **Time-Lapse & Scheduled Recording** — Implement scheduled recording based on configurable time windows (e.g., record only during work hours), time-lapse mode with adjustable interval capture, and calendar-based scheduling with recurring patterns. Store recordings with timestamp-based filenames for easy organization. `M`

11. [ ] **OTA Firmware Updates** — Add Over-The-Air update capability to upload new firmware via web interface without USB connection. Implement two-stage bootloader verification, rollback protection on failed updates, and update progress indication with automatic reboot on completion. `L`

12. [ ] **Performance Optimization & Caching** — Optimize frame processing pipeline with dual-core task distribution (detection on Core 0, web server on Core 1), implement frame buffer pooling to reduce memory allocation overhead, add configurable frame rate throttling based on available resources, and implement static asset caching with ETag support. `M`

> Notes
> - Items are ordered by strategic value to IoT developers prototyping vision applications
> - TensorFlow Lite integration (Item 1) is the highest priority based on user input
> - Each feature builds incrementally toward a complete vision prototyping platform
> - Current system already provides motion detection, web interface, file management, and health monitoring as foundation
> - Estimated efforts: XS=1 day, S=2-3 days, M=1 week, L=2 weeks, XL=3+ weeks
