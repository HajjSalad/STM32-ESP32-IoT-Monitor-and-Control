## IoT Monitor and Control with STM32 & ESP32

A scalable IoT solution combining STM32 for sensor simulation and ESP32 for cloud connectivity, built with FreeRTOS and AWS IoT Core.

## 🚀 Project Overview
A complete IoT demonstration platform featuring:
- **STM32** as sensor data generator (simulating digital sensors via HAL)
- **ESP32** as edge gateway with FreeRTOS real-time scheduling
- **AWS IoT Core** for secure cloud connectivity
- **Infrastructure-as-Code** provisioning with Terraform

---
### 🔑 Key Features:
🧪 **Sensor Simulation** [Link](https://github.com/HajjSalad/STM32-Sensor-Data-Simulation)   
&nbsp;&nbsp;&nbsp;• Simulated sensor readings using STM32 HAL, including ADC, PWM, and Timers.  
🔁 **Reliable Data Transfer**  
&nbsp;&nbsp;&nbsp;• Data transmission from STM32 to ESP32 via UART with a handshake mechanism.   
⏱️ **Real-Time Scheduling on ESP32**  
&nbsp;&nbsp;&nbsp;• Leveraging FreeRTOS for efficient real-time task scheduling and management.  
🧩 **Modular OOP Architecture**  
&nbsp;&nbsp;&nbsp;• Applied OOP principles to design a hierarchical class structure for sensor and device management.  
☁️ **Cloud Integration**    
&nbsp;&nbsp;&nbsp;• Data is transmitted to AWS IoT Core for real-time monitoring  
&nbsp;&nbsp;&nbsp;• AWS IoT Rules are used to store data in AWS Timestream for analytics.  

---
### 🧪 STM32 Sensor Node 



### 📂 STM32 Code Structure
```
├── 📁 STM32_Sensor_Node/                        # STM32 Sensor Node Firmware
│   ├── 📁 Src/                                  # Source files
│   │   ├── 📄 main.c                            # Main entry point, FreeRTOS scheduler
│   │   ├── 📄 syscalls.c                        # System call stubs for HAL/RTOS
│   │   ├── 📄 uart.c                            # UART driver implementation
│   │   ├── 📁 core/                             # Core device classes
│   │   │   ├── 📄 devices.cpp                   # Device management
│   │   │   ├── 📄 rooms.cpp                     # Room abstraction classes
│   │   │   ├── 📄 sensors.cpp                   # Sensor base classes
│   │   │   └── 📄 wrapper.cpp                   # C-compatible interfaces
│   │   └── 📁 tasks/                            # FreeRTOS tasks
│   │       ├── 📄 task_controller.c             # Main control task
│   │       ├── 📄 task_logger.c                 # Data logging task
│   │       ├── 📄 task_sensor_read.c            # Sensor read task
│   │       ├── 📄 task_sensor_write.c           # Sensor write task
│   │       └── 📄 task_transmit.c               # Data transmission task
│   │
│   ├── 📁 Inc/                                   # Header files
│   │   ├── 📁 CMSIS/                             # Cortex-M core headers
│   │   ├── 📁 core/                              # Core class headers
│   │   ├── 📁 tasks/                             # Task headers
│   │   ├── 📄 shared_resources.h                 # Shared variables and defines
│   │   └── 📄 uart.h                             # UART interface definitions
│   │
│   ├── 📁 FreeRTOS/                              # FreeRTOS kernel source and config
│   ├── 📁 Build/                                 # Build output folder
│   ├── 📁 Startup/                               # Startup code and vector table
│   ├── 📄 Makefile                               # Build rules
│   ├── 📄 STM32F446RETX_FLASH.ld                 # Flash linker script
│   └── 📄 STM32F446RETX_RAM.ld                   # RAM linker script         
│                      
```

---
### 📡 **Interrupt-Driven Handshake UART**
Ensures reliable data transfer between STM32 (transmitter) and ESP32 (receiver):
```
|         STM32                 |         ESP32                    |
|    Send: "READY?"             |                                  |
|                               |   Received: "READY?"             |
|                               |   Response: "YES"                |
|   Received: "YES"             |                                  |
|   Send: <DATA_PACKET>         |                                  |
|                               |   Received: <DATA_PACKET>        |
|                               |   Responded with: "ACK"          |
|   Received: "ACK"             |                                  |
|   [Transmission Complete]     |   [Process Data]                 |
```

---
### 🧱 Modular, Scalable Sensor & Device Architecture
🏠 `Room` (Base Class)  
&nbsp;&nbsp;&nbsp;• Abstract representation of a room within the system.  
&nbsp;&nbsp;&nbsp;• Specialized subclasses: LivingRoom, BedRoom.  
🌡️ `Sensor` (Base Class)  
&nbsp;&nbsp;&nbsp;• Generic interface for all sensor types.  
&nbsp;&nbsp;&nbsp;• Specialized subclasses: TempSensor, MotionDetector.  
🔌 `Device` (Base Class)  
&nbsp;&nbsp;&nbsp;• Common interface for all controllable devices.  
&nbsp;&nbsp;&nbsp;• Specialized subclasses: Light, AC, Heater.

#### 🧩 **Room Configuration**  
🪟 A `Room` can either be a `BedRoom` or a `LivingRoom`  
🚪 Each `LivingRoom` or `BedRoom` contains:  
&nbsp;&nbsp;&nbsp;• 1 `TempSensor`, 1 `MotionDetector`  
&nbsp;&nbsp;&nbsp;• 1 `Light`, 1 `AC`, 1 `Heater`  

💡 **Room Creation and Sensor Usage Example**  
```c
// Create a LivingRoom instance with a specific room number
int roomNum = 101;
void* room1 = createLivingRoom(roomNum);

if (!room1) {            // Check if room creation successful
    printf("LivingRoom creation failed.\n");
    return;
} else {
    printf("LivingRoom %d created.\n\r", roomNum);
}

// Sensor values (example data)
float tempValue = 27.5;
int motionValue = 1;  // 1 = motion detected, 0 = no motion

// Set sensor values
setTempSensorValue(room1, tempValue);
setMotionDetectorValue(room1, motionValue);
```
⏲️ **Device Control Based on Sensor Data**
```c
if (getMotionDetectorValue(room1)) {         // Turn on light if motion is detected
    turnOnLight(room1);
    printf("Light turned ON in Room %d\n\r", roomNum);
}

float temp = getTempSensorValue(room1);      // Read current temperature

// Control AC and Heater based on temperature range
if (temp > 25.0) {                           // Temp too hot
    turnOnAC(room1);
    turnOffHeater(room1);
    printf("AC turned ON in Room %d\n\r", roomNum);

} else if (temp < 20.0) {                    // Temp too cold
    turnOffAC(room1);
    turnOnHeater(room1);
    printf("Heater turned ON in Room %d\n\r", roomNum);

} else {                // Temperature is in comfortable range (20–25°C)
    turnOffAC(room1);
    turnOffHeater(room1);
    printf("Heater and AC turned OFF in Room %d\n\r", roomNum);
}
```

---
### 🏗 System Architecture
```
[STM32 (Simulate data)] → [UART] → [ESP32 (FreeRTOS & Cloud Gateway)] → [MQTT] → [Cloud Dashboard]
```

### 🛠️ Development Tools & Software
𐂷 **Microcontroller Development**  
&nbsp;&nbsp;&nbsp;⎔ **STM32 Development**  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;• STM32CubeIDE – Integrated development environment for STM32 firmware   
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;• ST-Link Debugger – Enables flashing and debugging over SWD      
&nbsp;&nbsp;&nbsp;⎔ **ESP32 Development**:  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;• ESP-IDF - Official development framework for ESP32 firmware  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;• VS Code - Development environment with ESP-IDF integration and UART debugging    
🌐 **Cloud Infrastructure**    
&nbsp;&nbsp;&nbsp;⎔ **AWS IoT Core** - Secure MQTT messaging and device connectivity     
&nbsp;&nbsp;&nbsp;⎔ **AWS Timestream** - Time-series database for storing and analyzing sensor data     
&nbsp;&nbsp;&nbsp;⎔ **Terraform** - Automates the provisioning and configuration of AWS infrastructure     
⚙️ **Hardware**  
&nbsp;&nbsp;&nbsp;⎔ **STM32 MCU** - Microcontroller used for real-time sensor data acquisition and local processing     
&nbsp;&nbsp;&nbsp;⎔ **ESP32 MCU** - Acts as the cloud gateway, handling connectivity and communication with AWS   

### Hardware Connection
```
|       STM32 PIN       |    Interface     |     ESP32 Pin             |  
|    PA9 - USART1_TX    |      UART        |     GPIO17 - UART2_TX     |  
|    PA10 - USART1_RX   |      UART        |     GPIO16 - UART2_RX     |  
|        GND            |      GND         |      GND                  |  
```
---
### 📂 ESP32 Code Structure
```
├── 📁 ESP32_Cloud_Gateway/                 # ESP32 Gateway Firmware
│   ├── 📁 main/                            # Core FreeRTOS tasks and entry point
│   │   ├── 📄 main.c                       # Main program, FreeRTOS scheduler and tasks
│   │   ├── 📄 CMakeLists.txt               # Build configuration for main folder
│   │   └── 📁 include/                     # Public headers for main tasks
│   │       └── 📄 task_priorities.h        # Task priority definitions
│   │
│   ├── 📁 components/                      # Modular firmware components
│   │   ├── 📁 mqtt/                        # MQTT communication module
│   │   │   ├── 📄 CMakeLists.txt           # Build configuration for MQTT component
│   │   │   ├── 📄 cloud_mqtt_task.c        # FreeRTOS task for MQTT communication
│   │   │   ├── 📄 mqtt_driver.c            # Core MQTT driver implementation
│   │   │   ├── 📁 include/                 # MQTT public headers
│   │   │   │   └── 📄 mqtt.h               # MQTT interface definitions
│   │   │   └── 📁 certs/                   # Certificates for AWS IoT Core
│   │   │
│   │   ├── 📁 uart/                        # UART communication module
│   │   │   ├── 📄 CMakeLists.txt           # Build configuration for UART component
│   │   │   ├── 📄 uart2_driver.c           # UART driver for hardware communication
│   │   │   ├── 📄 uart_rxtx_task.c         # FreeRTOS task for UART RX/TX
│   │   │   └── 📁 include/                 # UART public headers
│   │   │       └── 📄 uart.h               # UART interface definitions
│   │   │
│   │   └── 📁 wifi/                        # WiFi connectivity module
│   │       ├── 📄 CMakeLists.txt           # Build configuration for WiFi component
│   │       ├── 📄 wifi_driver.c            # Core WiFi driver implementation
│   │       ├── 📄 wifi_manager_task.c      # FreeRTOS task for WiFi management
│   │       └── 📁 include/                 # WiFi public headers
│   │           └── 📄 wifi.h               # WiFi interface definitions
│   │
│   └── 📄 CMakeLists.txt                   # Top-level build system configuration
│
└── 📄 README.md                            # Project documentation
```

### Diagram
![Pic](./IoTSystemDiagram.png)

#### Demo
&nbsp;&nbsp;&nbsp;AWS IoT Core&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;&#8195;Serial Terminal
![Demo](./png_demo.gif)
