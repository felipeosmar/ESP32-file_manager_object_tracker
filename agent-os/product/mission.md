# Product Mission

## Pitch
ESP32 Vision Prototyper is an embedded computer vision platform that helps IoT developers and makers prototype vision applications quickly by providing a web-based development environment with integrated object tracking, file management, and extensible ML capabilities on affordable ESP32 hardware.

## Users

### Primary Customers
- **IoT Developers**: Professionals building smart cameras, security systems, and automation products who need rapid prototyping capabilities
- **Makers & Hobbyists**: Electronics enthusiasts experimenting with computer vision and robotics projects
- **Students & Educators**: Learning embedded systems, computer vision, and IoT development on accessible hardware

### User Personas

**Alex, IoT Product Developer** (28-45 years old)
- **Role:** Embedded Systems Engineer at a smart home startup
- **Context:** Building proof-of-concepts for vision-enabled security cameras and needs to iterate quickly on detection algorithms
- **Pain Points:**
  - Setting up development environments for embedded vision is time-consuming
  - Limited debugging capabilities on embedded devices
  - Difficult to demonstrate prototypes to stakeholders without complex setups
  - Need to balance functionality with tight hardware resource constraints
- **Goals:**
  - Validate computer vision concepts before investing in production hardware
  - Quickly adjust detection parameters and test different algorithms
  - Deploy working demos that non-technical stakeholders can interact with via web interface

**Jordan, Maker & Robotics Enthusiast** (22-40 years old)
- **Role:** Hobbyist building DIY robotics and automation projects
- **Context:** Creating home automation projects, pet monitoring systems, and learning robotics
- **Pain Points:**
  - Computer vision requires expensive hardware and complex software stacks
  - Steep learning curve for ML frameworks and embedded programming
  - Limited budget for prototyping multiple concepts
- **Goals:**
  - Build functional vision-based projects on affordable hardware
  - Learn computer vision and embedded ML without overwhelming complexity
  - Share and replicate projects with the maker community

**Sam, Embedded Systems Instructor** (30-55 years old)
- **Role:** University professor or technical workshop instructor
- **Context:** Teaching IoT, embedded systems, and computer vision courses
- **Pain Points:**
  - Need accessible, affordable hardware for student projects
  - Students struggle with complex toolchains and development setup
  - Difficult to provide hands-on experience with vision systems in classroom settings
- **Goals:**
  - Provide students with practical, hands-on computer vision experience
  - Enable students to see results quickly to maintain engagement
  - Use platform that students can replicate for personal projects

## The Problem

### Embedded Computer Vision Has High Barriers to Entry

Prototyping computer vision applications on embedded devices typically requires expensive hardware (Raspberry Pi with cameras, Jetson Nano, specialized vision boards costing $100+), complex software setup (OpenCV, TensorFlow installation, cross-compilation), and significant development time just to get a basic system running. This creates a 2-4 week delay before developers can even begin testing their actual vision algorithms, and makes experimentation prohibitively expensive for makers and students.

**Our Solution:** Provide a complete, ready-to-run vision platform on affordable ESP32-CAM hardware ($10-15) with web-based configuration and debugging. Developers can start testing motion detection and object tracking immediately, then progressively add ML capabilities as their prototype matures.

### No Integrated Development Environment for Embedded Vision

Traditional embedded vision development requires switching between multiple tools: flashing firmware via USB, monitoring serial output in one terminal, serving web interfaces from another process, and editing configuration files manually. This fragmented workflow makes iteration slow and debugging difficult, especially when trying to tune detection parameters or test different tracking algorithms.

**Our Solution:** Integrate all development tools into a single web interface - live camera preview, file editing directly on the SD card, real-time system health monitoring, and interactive parameter adjustment. No USB cable or terminal windows required once initial deployment is complete.

### Limited Resources on Embedded Devices Restrict Experimentation

ESP32 and similar microcontrollers have limited RAM (520KB), flash storage (4MB), and processing power compared to single-board computers. This makes it challenging to implement sophisticated computer vision algorithms, store ML models, or even save recorded footage for analysis. Developers often hit resource limits early in prototyping and must restart with more expensive hardware.

**Our Solution:** Implement efficient motion detection using frame differencing and downsampling (80x60 pixels for processing), provide SD card integration for model storage and data logging, and create a modular architecture where TensorFlow Lite models can be swapped without firmware recompilation. This maximizes what can be achieved within ESP32 constraints while keeping upgrade paths open.

## Differentiators

### Web-First Development Experience
Unlike traditional embedded systems that require USB connections, serial monitors, and recompilation for configuration changes, we provide complete development through a web browser. Adjust tracking parameters, edit files, monitor system health, and control the camera remotely without touching the device. This makes prototyping faster and enables remote debugging and demonstration.

### Progressive Enhancement Architecture
Most embedded vision platforms are either basic motion detection OR complex ML systems. We start with efficient motion tracking that works out-of-the-box, then allow developers to progressively add TensorFlow Lite models, custom detection algorithms, and integration with home automation systems as their needs evolve. Prototypes can grow from simple motion tracking to sophisticated object recognition without platform migration.

### Optimized for Resource-Constrained Hardware
While Raspberry Pi solutions offer more processing power, they cost 3-5x more and consume significantly more power (2.5W vs 0.5W). We achieve practical computer vision on ESP32 through intelligent optimization: frame downsampling for motion detection, efficient PID control for tracking, mutex-protected SD card operations, and careful memory management. This makes vision projects accessible on a $15 hardware budget with battery-powered operation feasible.

### Integrated File Management System
Unlike competitors requiring separate SPIFFS upload tools or USB file transfers, our integrated web-based file manager allows direct editing of HTML/CSS/JS interface files, ML model replacement, and configuration management from any device with a browser. This dramatically shortens the development cycle and enables non-technical users to customize interfaces without programming knowledge.

## Key Features

### Core Features
- **Real-Time Motion Detection & Tracking:** Detect moving objects using efficient frame differencing algorithms and automatically track them with pan/tilt servo control, providing smooth following behavior with PID control tuning.

- **Web-Based Interface:** Complete control interface accessible from any browser on the network, featuring live camera stream, manual servo control, automatic tracking toggle, and real-time configuration adjustment.

- **SD Card File Management:** Built-in file manager accessible via web browser enables viewing, editing, uploading, and deleting files directly on the SD card, eliminating the need for USB file transfers or SPIFFS upload tools.

### Development & Monitoring Features
- **System Health Dashboard:** Real-time monitoring of memory usage (heap and PSRAM), WiFi signal strength, SD card capacity, CPU frequency, and system uptime, helping developers identify performance bottlenecks and resource constraints during prototyping.

- **In-Browser File Editor:** Edit HTML, CSS, JavaScript, JSON configuration files, and text-based assets directly through the web interface with a 50KB file size limit optimized for ESP32 memory constraints.

- **JSON-Based Configuration:** All system parameters (WiFi credentials, tracking sensitivity, servo limits, PID gains) stored in a human-readable JSON file on SD card, enabling quick configuration changes without firmware recompilation.

### Advanced Features
- **TensorFlow Lite Integration (Roadmap Priority 1):** Add pre-trained or custom ML models for object classification, face detection, or custom vision tasks, with models loaded from SD card for easy experimentation and swapping.

- **Multi-Mode WiFi Operation:** Support both Access Point mode (for standalone operation) and Station mode (for network integration), with automatic fallback to AP mode if station connection fails.

- **RESTful API:** Programmatic control of tracking, servo positions, and system configuration via HTTP API, enabling integration with home automation systems, MQTT brokers, or custom applications.

- **Video Recording & Playback (Future):** Store video clips to SD card when motion is detected, with web-based playback and download capabilities for post-analysis and sharing.

- **MQTT & Home Assistant Integration (Future):** Publish detection events and camera snapshots to MQTT topics, with Home Assistant discovery support for seamless smart home integration.
