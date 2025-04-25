## IoT System with STM32 and ESP32

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
&nbsp;&nbsp;&nbsp;• Data is transmitted to AWS IoT Core for real-time monitoring, and AWS IoT Rules are used to store data in AWS Timestream for analytics.  

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

---
### 🏗 System Architecture
```
[STM32 (Simulate data)] → [UART] → [ESP32 Cloud Gateway] → [MQTT] → [Cloud Dashboard]
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


---
### 📂 Project Code Structure
```
📁 Smart-Fire-Detection-System/
│── 📁 stm32_sensor_node/
│   ├── 📄 main.c               (Entry point of the program)
│   ├── 📄 factory.cpp / .h     (Abstract Factory pattern implementation)
│   ├── 📄 sensor.cpp / .h      (Base sensor classes and interfaces)
│   ├── 📄 wrapper.cpp / .h     (Hardware abstraction layer wrappers)
│   ├── 📄 simulate.c / .h      (Sensor data simulation)
│   ├── 📄 spi.c / .h           (SPI & GPIO Interrupt Communication)
│   ├── 📄 uart.c / .h          (UART Communication)
│   ├── 📄 systick.c / .h       (Systick Timer)
│   ├── 📄 Makefile             (Build system configuration)
│── 📁 esp32_facp_cloud_node/
│   ├── 📄 main.c               (Entry point of the program, Tasks)
│   ├── 📄 spi.c / .h           (SPI & GPIO Interrupt Communication )
│   ├── 📄 uart.c / .h          (UART Communication)
│   ├── 📄 wifi.c / .h          (WiFi Connectivity)
│   ├── 📄 cloud.c / .h         (MQTT for AWS Connectivity)
│   ├── 📄 CMakeLists.txt       (Build system configuration)
│── 📄 README.md  (Documentation)
```

### Diagram
![Pic](./IoTSystemDiagram.png)

#### Demo
![Demo](./IoTSystemGIF.gif)
