# IoT System with STM32 and ESP32

This project demonstrates the design and development of an IoT system using STM32 and ESP32 microcontrollers. The system involves sensor simulation on STM32, real-time task scheduling on ESP32 with FreeRTOS, and cloud integration via AWS IoT Core.

## Project Overview

The system integrates STM32 and ESP32 to simulate sensor readings, transmit data, and store it in the cloud for real-time monitoring and analytics. The STM32 uses the Hardware Abstraction Layer (HAL) to simulate sensor data, while the ESP32 handles data processing and cloud communication via WiFi and HTTP protocols.

### Key Features:
- **Sensor Simulation on STM32**: Simulated sensor readings using STM32 HAL, including ADC, PWM, and Timers.
- **Reliable Data Transfer**: Data transmission from STM32 to ESP32 via UART with a handshake mechanism.
- **Real-Time Scheduling on ESP32**: Leveraging FreeRTOS for efficient real-time task scheduling and management.
- **Object-Oriented Design**: Applied OOP principles to design a hierarchical class structure for sensor and device management.
- **Cloud Integration**: Data is transmitted to AWS IoT Core for real-time monitoring, and AWS Lambda is used to store data in AWS Timestream for analytics.
  
## Tools & Software

- **STM32 Development**:
  - STM32CubeIDE (with HAL for configuration)
  - ST-Link Debugger
- **ESP32 Development**:
  - ESP-IDF
  - VS Code (with UART Debugging)
- **Cloud**:
  - AWS IoT Core
  - AWS Lambda
  - AWS Timestream
- **Hardware**:
  - STM32 MCU
  - ESP32 MCU

## System Architecture

- **STM32**: Simulates sensor data and transmits it to the ESP32 over UART.
- **ESP32**: Handles real-time scheduling with FreeRTOS, manages sensors, and communicates with the cloud (AWS IoT Core).
- **Cloud**: AWS IoT Core handles data reception, AWS Lambda processes the data, and AWS Timestream stores and analyzes time-series data.

## Communication Flow

1. **STM32 sends READY to ESP32**.
2. **ESP32 responds with OK**.
3. **STM32 sends sensor data to ESP32**.
4. **ESP32 responds with ACK**.
5. **Transmission complete**.

## Setup

### Hardware Setup:
- Connect the STM32 and ESP32 via UART.
- Ensure both boards are powered and connected to your development environment.

### Software Setup:

1. **STM32:**
   - Open the project in STM32CubeIDE.
   - Configure the microcontroller using HAL, set up timers, PWM, and ADC.
   - Flash the code to the STM32 using the ST-Link Debugger.

2. **ESP32:**
   - Open the project in Visual Studio Code with ESP-IDF.
   - Configure FreeRTOS for real-time task scheduling.
   - Set up WiFi connectivity and HTTP protocol to communicate with AWS IoT Core.

3. **AWS IoT Core:**
   - Set up AWS IoT Core for device management and cloud integration.
   - Configure AWS Lambda to store sensor data in AWS Timestream.

### Building & Flashing:
1. Compile the STM32 firmware using STM32CubeIDE and flash it onto the STM32.
2. Compile the ESP32 firmware using ESP-IDF and flash it onto the ESP32.

## Usage

1. Upon powering on, the STM32 will begin simulating sensor data.
2. The STM32 will transmit data to the ESP32 over UART.
3. The ESP32 will process the data and send it to AWS IoT Core via HTTP.
4. The data will be stored in AWS Timestream for real-time analytics and monitoring.

## Future Enhancements

- **Data Visualization**: Implement a dashboard to visualize the sensor data stored in AWS Timestream.
- **Additional Sensors**: Integrate additional sensors with the STM32 for more data sources.
- **Machine Learning**: Implement anomaly detection using machine learning models on the ESP32 or in the cloud.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
