## 🌐 IoT Monitor and Control
## Table of Contents
## Table of Contents
- [Overview](#overview)
- [🟠 STM32 Sensor Node](#-stm32-sensor-node)
  - [💾 Memory Footprint - STM32F446RE](#-memory-footprint---stm32f446re)
    - [Task Stacks](#task-stacks)
  - [🔬 Sensor Stack](#-sensor-stack)
  - [🧱 Object Model](#-object-model)
  - [🧵 FreeRTOS Task Pipeline](#-freertos-task-pipeline)
  - [🔗 FreeRTOS Resources](#-freertos-resources)
  - [📡 Peripheral Drivers](#-peripheral-drivers)
- [🔵 UART Transport Protocol](#-uart-transport-protocol)
  - [📦 Packet Structure](#-packet-structure)
  - [🗳️ Packet Types](#️-packet-types)
  - [📜 Transaction](#-transaction)
- [🔴 ESP32 Cloud Gateway](#-esp32-cloud-gateway)
  - [🧵 Task Model](#-task-model)
  - [🔗 FreeRTOS Resources](#-freertos-resources-1)
- [⚙️ Hardware Connection & Sensor Wiring](#️-hardware-connection--sensor-wiring)
- [📂 STM32 Code Structure](#-stm32-code-structure)
- [📂 ESP32 Code Structure](#-esp32-code-structure)

#### Overview
A two-MCU embedded IoT system that monitors temperature and motion in a room,
drives climate and lighting control, and publishes telemetry to AWS IoT Core.
An STM32F446RE runs the sensor and control logic via FreeRTOS, forwarding data
over a custom binary UART protocol to an ESP32-WROOM-32E cloud gateway that
manages Wi-Fi connectivity, MQTT/TLS publishing, and OTA firmware updates via
AWS IoT Jobs and S3.

The system is split into three components:
- **🟠 STM32 Sensor Node** - FreeRTOS firmware, C++ object model, sensor sampling, control logic
- **🔵 UART Transport Protocol** - custom binary packet protocol with state machines, CRC and ACK/retry
- **🔴 ESP32 Cloud Gateway** - Wi-Fi, MQTT/TLS publishing, OTA firmware updates       

📓 Design notes, dev journal, and setup guides (STM32 & ESP32 project creation, design decisions) are documented in the [Project Notion Page](https://hajjsalad.notion.site/iot-monitor-control)   

![overview](./project_diagram.png)
---
### 🟠 STM32 Sensor Node 
FreeRTOS-based sensor and control firmware running on an STM32F446RE,
structured around a C++ object model wrapped in a C interface. Models the
room using a class hierarchy - sensors and actuators are represented as
objects, with an `extern "C"` wrapper layer bridging the C++ object model to
the C-based FreeRTOS task API. Monitors temperature and motion, drives
climate and lighting control, and forwards sensor data and device state to an
ESP32 gateway over a custom UART protocol.

#### 💾 Memory Footprint - STM32F446RE
| Memory | Size | Address | Content |
|---|---|---|---|
| Flash | 512 KB | 0x08000000 | Code, constants, initialized data values |
| SRAM | 128 KB | 0x20000000 | Runtime data, FreeRTOS heap, stack |

**Heap Configuration**
Selected `heap_4.c` for the heap allocation strategy - only one heap implementation can be linked at a time:  
```
FreeRTOS/Source/portable/MemMang/heap_4.c
```
`heap_4.c` allocates/frees and coalesces adjacent free blocks, reducing fragmentation over long uptime - the standard choice for most embedded projects with dynamic FreeRTOS object creation.   

Heap size set in `FreeRTOSConfig.h`:
```
#define configTOTAL_HEAP_SIZE    ( ( size_t ) ( 55000 ) )
```

**Heap Usage at Runtime**
Printed at startup via `xPortGetFreeHeapSize()`:
| Metric | Value |
|---|---|
| FreeRTOS heap total | 55,000 |
| FreeRTOS heap remaining | 3,928 bytes |
| FreeRTOS heap consumed | 51,072 |

**Binary Size**  
```
arm-none-eabi-size Build/STM32_Sensor_Node.elf
  text    data     bss     dec     hex
  24296    96     57680   82072   14098
```
| Section | Size (bytes) | Stored In | Notes |
|---|---|---|---|
| `.text` | 24,296 | Flash | Firmware code + FreeRTOS source |
| `.data` | 96 | Flash + SRAM | Initial values in flash, copied to SRAM at boot |
| `.bss` | 57,680 | SRAM | Includes FreeRTOS `ucHeap[55000]` + other uninitialized globals |

```
Flash used:   
text + data = 24,296 + 96 = 24,392 bytes   
24,392 / (512 × 1024) = 24,392 / 524,288 = 4.65%
SRAM used:
data + bss = 96 + 57,680 = 57,776 bytes
57,776 / (128 × 1024) = 57,776 / 131,072 = 44.08%
SRAM free:
128 KB -> 128 × 1024 = 131,072 bytes
131,072 - 57,776 = 73,296 bytes ≈ 71.6 KB
```
| | Used | Total | % |
|---|---|---|---|
| Flash | 24,392 | 512 KB | 4.65 |
| SRAM | 57,776 | 128 KB | 44.08 |

##### Task Stacks
```
UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
```
| Task | Stack (words) | Stack (bytes) | Priority | HWM Remaining (words) | Used (words) | Why |
|---|---|---|---|---|---|---|
| `vTaskSensorSample` | 512 | 2,048 | 8 | | | |
| `vTaskSensorRead` | 512 | 2,048 | 7 | | | |
| `vTaskController` | 512 | 2,048 | 6 | | | |
| `vTaskRouter` | 2,048 | 8,192 | 5 | | | |
| `vTaskTX` | 4,096 | 16,384 | 4 | | | |
| `vTaskRX` | 2,048 | 8,192 | 4 | | | |
| `vTaskLogger` | 1,024 | 4,096 | 3 | | | |
| `vTaskWatchdogMonitor` | 512 | 2,048 | 9 | | | |
| **Total** | **11,264** | **45,056** | | | | |

**FreeRTOS Objects**  
| Object | Count × Size | Total |
|---|---|---|
| Task Control Blocks | 8 × ~88 bytes | |
| `xSensorQueue` | | |
| `xTXQueue` | | |
| `xRXQueue` | | |
| `xLogQueue` | | |
| `xSensorMutex` | | |
| `xUartStreamBuffer` | | |
| `xSampleTimer` | | |
| **Approximate total consumed** | | |

#### 🔬 Sensor Stack
| Sensor | Measurement | Interface | Status |
|---|---|---|---|
| TMP102 | Temperature | I2C2 | Real hardware |
| PIR | Motion detected / not detected | GPIO Input + EXTI12 | Real hardware |

#### 🧱 Object Model
**Class Hierarchy & Composition**     
Sensors and devices are grouped into typed inheritance hierarchies, composed together inside a `Room`:
```
Sensor (abstract)           Device (abstract)           Room
├── MotionDetector          ├── Light                   ├── MotionDetector    (1)
└── TemperatureSensor       ├── AC                      ├── TemperatureSensor (1)
                            └── Heater                  ├── Light             (1)
                                                        ├── AC                (1)
                                                        └── Heater            (1)
```
`Room` is a concrete aggregate that owns one instance of every sensor and device type and exposes a unified control interface.    
New sensor or device types can be added by extending the base classes, and new room types by deriving from `Room` - without modifying existing code.   

#### 🧵 FreeRTOS Task Pipeline
| Task | Priority | Role |
|---|---|---|
| `vTaskSensorSample` | 8 | Samples TMP102 + PIR, writes to Room object |
| `vTaskSensorRead` | 7 | Reads from Room object, posts to `xSensorQueue` |
| `vTaskController` | 6 | Applies control logic, drives actuators |
| `vTaskRouter` | 5 | Drains UART stream buffer, frames packets, routes to TX/RX |
| `vTaskTX` | 4 | UART transmit state machine - handshake, data, ACK/retry |
| `vTaskRX` | 4 | Handles incoming commands from ESP32, sends ACKs |
| `vTaskLogger` | 3 | Drains log queue, outputs over UART2 |
| `vTaskWatchdogMonitor` | 9 | Checks task alive flags, kicks IWDG |

#### 🔗 FreeRTOS Resources
| Resource | Type | Purpose |
|---|---|---|
| `xSensorMutex` | Mutex | Guards shared `Room` object between `SensorSample` and `SensorRead` |
| `xSensorQueue` | Queue | Passes `SensorData_t` from `SensorRead` → `Controller` |
| `xTXQueue` | Queue | Passes `TXQueue_Item_t` from `Controller` → `TX` |
| `xRXQueue` | Queue | Passes incoming packets from `Router` → `RX` |
| `xLogQueue` | Queue | Passes log strings from all tasks → `Logger` |
| `xUartStreamBuffer` | Stream Buffer | Carries raw UART byte stream from ISR → `Router` |
| `xSampleTimer` | Software Timer | Triggers `SensorSample` every 5 seconds |
| `xSensorSampleHandle` | Task Handle | Notified by `xSampleTimer` callback |
| `xSensorReadHandle` | Task Handle | Notified by `SensorSample` after writing to `Room` |
| `xTXTaskHandle` | Task Handle | Notified by `Router` on incoming ACK packets |

#### 📡 Peripheral Drivers 
**`UART1` - ESP32 Communication**   
Bare-metal UART1 driver at 115200 baud. ISR receives bytes one at a time and
posts them into `xUartStreamBuffer` via `xStreamBufferSendFromISR`. Trigger
level of 8 bytes wakes `vTaskRouter`, which drains the buffer in chunks of 32
and frames packets using SOF (`0xAA`) / EOF (`0x55`) boundary detection.
```
PA9  — TX (AF7)
PA10 — RX (AF7)
```
**`UART2` - Debug Logging**   
Dedicated UART for terminal debug output. `vTaskLogger` is the sole writer - drains `xLogQueue` and transmits log messages without blocking other tasks.
```
PA2 - TX (AF7)
PA3 - RX (AF7)
```
**`I2C2` - TMP102 Temperature Sensor**   
Bare-metal I2C2 master driver at 100kHz standard mode. `vTaskSensorSample`
issues a read transaction each sample cycle and writes the result into the
`Room` object's `TemperatureSensor` via the C++ wrapper layer.
```
PB10 - SCL
PB11 - SDA
Address: 0x48
```
**`GPIO Input` - PIR Motion Sensor**      
PB12 configured as digital input with internal pull-up resistor. EXTI12
interrupt on falling edge. Falling edge sets a `volatile` motion flag inside
the ISR; `vTaskSensorSample` reads and clears it each cycle alongside the
temperature sensor - no polling required.
```
PB12 - EXTI12
pull-up, falling edge
```
**`IWDG` - Independent Watchdog**       
Hardware watchdog clocked by internal LSI oscillator (32kHz) - independent of system clock, cannot be disabled once started. `vTaskWatchdogMonitor` (Pri 7, highest) verifies all four tasks set their alive flags each cycle before kicking. If any task hangs and fails to set its flag - kick is withheld and MCU resets after timeout.
```
Prescaler  = /256  (PR = 6)
Reload     = 1250  (RLR)
Timeout    = (256 × 1250) / 32000 = 10 seconds
```
**`RTC` - Sampling Clock**   
Independent BCD timer/counter clocked by the internal LSI oscillator (32kHz).
Provides accurate timestamps for each sensor sample. Currently used for
timestamping.   

---
### 🔵 **UART Transport Protocol**
Custom full-duplex bidirectional binary protocol designed for reliable
communication between the STM32 sensor node and the ESP32 gateway over a
single UART link. Evolved through five design stages, currently implementing
Stage 3 with Stage 5 as the long-term target.
```
Stage 1 - raw byte stream
Stage 2 - structured packets, no validation
Stage 3 - CRC16 validation, ACK/retry         
Stage 4 - sequence numbering, flow control    <- current
Stage 5 - full-duplex, QoS levels, HAL layer  <- target
```
#### 📦 Packet Structure
```
| SOF(0xAA) | VER | DEV_ID | SEQ | TYPE | LEN | PAYLOAD | CRC16_H | CRC16_L | EOF(0x55) |
```
| Field | Size | Description |
|---|---|---|
| `SOF` | 1 byte | Start of frame - fixed `0xAA` |
| `VER` | 1 byte | Protocol version |
| `DEV_ID` | 1 byte | Sender device ID (STM32 / ESP32) |
| `SEQ` | 1 byte | Sequence number, 7-bit (`0x00`-`0x7F`) |
| `TYPE` | 1 byte | Packet type - see table below |
| `LEN` | 1 byte | Payload length (data packets only) |
| `PAYLOAD` | variable | Packet-specific data |
| `CRC16_H/L` | 2 bytes | CRC16 over header + payload |
| `EOF` | 1 byte | End of frame - fixed `0x55` |
#### 🗳️ Packet Types
| Direction | Type | Purpose |
|---|---|---|
| Either | `PKT_HANDSHAKE_REQ` | Initiates a transaction |
| Either | `PKT_HANDSHAKE_ACK` | Acknowledges handshake |
| STM32 → ESP32 | `PKT_TEMP_DATA` | Temperature reading |
| STM32 → ESP32 | `PKT_MOTION_DATA` | Motion detection state |
| STM32 → ESP32 | `PKT_MESSAGE_DATA` | General message payload |
| ESP32 → STM32 | `PKT_COMMAND` | Setpoint / threshold update |
| ESP32 → STM32 | `PKT_FIRMWARE` | OTA firmware chunk |
| Either | `PKT_*_ACK` | Acknowledges matching data packet |
#### 📜 Transaction
Each transaction tracks `seq`, `payloadType`, `retries`, `start_tick`/`end_tick`,
and `result` (`PENDING`/`SUCCESS`/`FAILED`) in a `TX_Transaction_t` struct.
Sequence number increments on both success and failure, guaranteeing
monotonically increasing sequence numbers regardless of outcome.   

Sequence numbers are split into two ranges so STM32 and ESP32 can each
generate sequence numbers independently without collision:
```
SEQ_STM32_BASE  0x00   // STM32-originated: 0x00, 0x01, 0x02 ... 0x7F
SEQ_ESP32_BASE  0x80   // ESP32-originated: 0x80, 0x81, 0x82 ... 0xFF
```
The high bit of `SEQ` doubles as an origin marker - `0x00`–`0x7F` identifies
a STM32-initiated transaction, `0x80`–`0xFF` identifies an ESP32-initiated
transaction, allowing either side to immediately tell who started a given
exchange just by inspecting the sequence byte.   

![uart](./uart_protocol.png)   

---
### 🔴 ESP32 Cloud Gateway
The ESP32 acts as a cloud gateway — receiving sensor data from the STM32 over a custom UART protocol, managing Wi-Fi connectivity, and publishing telemetry to AWS IoT Core over MQTT/TLS

#### 🧵 Task Model
| Task | Responsibility |
|---|---|
| `uart_router_task` | Drains UART2 `uart_2_queue`, frames/validates packets, routes by type |
| `uart_tx_task` | Blocks on `tx_queue`, sends packets to STM32, manages ACK/retry state machine |
| `uart_rx_task` | Blocks on `rx_queue`, handles incoming packets from STM32, sends ACKs |
| `wifi_manager_task` | Connects to AP in station mode, monitors connection, reconnects on drop |
| `cloud_mqtt_task` | Connects to AWS IoT Core over TLS, publishes telemetry, subscribes to OTA job topics |
| `ota_task` | Waits for job notification, parses job document, downloads + flashes new firmware |

#### 🔗 FreeRTOS Resources
| Resource | Type | Purpose |
|---|---|---|
| `uart_2_queue` | Queue | UART driver event queue - triggers `uart_router_task` on incoming data |
| `tx_queue` | Queue | Passes outgoing packets to `uart_tx_task` |
| `rx_queue` | Queue | Passes incoming packets from `uart_router_task` -> `uart_rx_task` |
| `wifi_event_group` | Event Group | Signals Wi-Fi connection status - `WIFI_CONNECTED_BIT` |
| `mqtt_event_group` | Event Group | Signals MQTT connection status - `MQTT_CONNECTED_BIT` |

---
#### ⚙️ Hardware Connection & Sensor Wiring
```
|       STM32 PIN       |    Interface     |     ESP32 Pin             |  
|    PA9  - USART1_TX   |      UART        |     GPIO16 - UART2_RX     |  
|    PA10 - USART1_RX   |      UART        |     GPIO17 - UART2_TX     |  
|        GND            |      GND         |           GND             |

|     STM32 Pin         |    Interface     |     TMP102 Pin            |
|   PB10 - I2C2_SCL     |       I2C        |        SCL                |
|   PB11 - I2C2_SDA     |       I2C        |        SDA                |
|        3.3V           |      Power       |        VCC                |
|        GND            |       GND        |        GND                |

|       STM32 Pin       |    Interface     |      PIR Sensor Pin       |
|   PB12 - EXTI12       |    GPIO Input    |         OUT               |
|        3.3V           |      Power       |         VCC               |
|        GND            |       GND        |         GND               |
```

#### 📂 STM32 Code Structure
```
├── 📁 STM32_Sensor_Node/                        # STM32 Sensor Node Firmware
│   ├── 📁 Src/                                  # Source files
│   │   ├── 📄 main.c                            # Main entry point, FreeRTOS scheduler
│   │   ├── 📄 syscalls.c                        # System call stubs for newlib
│   │   │
│   │   ├── 📁 comm/                             # Communication peripheral drivers
│   │   │   ├── 📄 uart1_driver.c                # UART1 - ESP32 communication
│   │   │   ├── 📄 uart2_driver.c                # UART2 - debug logging
│   │   │   ├── 📄 i2c1_driver.c                 # I2C - TMP102 temperature sensor
│   │   │   └── 📄 exti_driver.c                 # EXTI - PIR motion sensor interrupt
│   │   │
│   │   ├── 📁 sensors/                          # Sensor-specific read logic
│   │   │   ├── 📄 tmp102_temp_sensor.c          # TMP102 read + conversion
│   │   │   └── 📄 button_motion_sensor.c        # PIR motion flag handling
│   │   │
│   │   ├── 📁 system/                           # System-level peripheral drivers
│   │   │   ├── 📄 rtc_driver.c                  # RTC - BCD time/date, LSI clock
│   │   │   └── 📄 iwdg_driver.c                 # IWDG - independent watchdog
│   │   │
│   │   ├── 📁 core/                             # C++ object model
│   │   │   ├── 📄 devices.cpp                   # Device base
│   │   │   ├── 📄 rooms.cpp                     # Room composition class
│   │   │   ├── 📄 sensors.cpp                   # Sensor base + Temp/Motion classes
│   │   │   └── 📄 wrapper.cpp                   # extern "C" bridge to FreeRTOS tasks
│   │   │
│   │   ├── 📁 tasks/                            # FreeRTOS tasks
│   │   │   ├── 📄 task_1_sensor_sample.c        # Samples sensors, writes to Room object
│   │   │   ├── 📄 task_2_sensor_read.c          # Reads Room object, posts to sensor queue
│   │   │   ├── 📄 task_3_controller.c           # Control logic, drives actuators
│   │   │   ├── 📄 task_4_uart_tx.c              # UART TX state machine
│   │   │   ├── 📄 task_5_uart_router.c          # Frames packets, routes TX/RX
│   │   │   ├── 📄 task_6_uart_rx.c              # Handles incoming commands, sends ACKs
│   │   │   ├── 📄 task_7_logger.c               # Drains log queue to UART2
│   │   │   └── 📄 task_8_watchdog.c             # Checks alive flags, kicks IWDG
│   │   │
│   │   └── 📁 utils/                            # Shared utilities
│   │       └── 📄 crc_16.c                      # CRC16-CCITT implementation
│   │
│   ├── 📁 Inc/                                   # Header files
│   │   ├── 📁 CMSIS/                             # Cortex-M core headers
│   │   ├── 📁 STM32F4xx/                         # Device register headers
│   │   ├── 📁 comm/                              # Communication driver headers
│   │   ├── 📁 sensors/                           # Sensor driver headers
│   │   ├── 📁 system/                            # RTC/IWDG headers
│   │   ├── 📁 core/                              # C++ object model headers
│   │   ├── 📁 tasks/                             # Task headers
│   │   ├── 📁 utils/                             # CRC header
│   │   └── 📄 shared_resources.h                 # Shared queues, mutexes, structs
│   │
│   ├── 📁 Tests/                                  # Unit tests (host-compiled)
│   ├── 📁 FreeRTOS/                               # FreeRTOS kernel source and config
│   ├── 📁 Build/                                  # Build output folder
│   ├── 📁 Startup/                                # Startup code and vector table
│   ├── 📄 Makefile                                # Build rules
│   ├── 📄 STM32F446RETX_FLASH.ld                  # Flash linker script
│   └── 📄 STM32F446RETX_RAM.ld                    # RAM linker script
```
#### 📂 ESP32 Code Structure
```
├── 📁 ESP32_Cloud_Gateway/                 # ESP32 Gateway Firmware
│   ├── 📁 main/                            # Entry point
│   │   ├── 📄 main.c                       # app_main - system init, creates all component tasks
│   │   └── 📄 CMakeLists.txt               # Build configuration for main folder
│   │
│   ├── 📁 components/                      # Modular firmware components
│   │   ├── 📁 wifi/                        # Wi-Fi connectivity component
│   │   │   ├── 📄 CMakeLists.txt
│   │   │   ├── 📄 wifi_driver.c            # Station mode init, esp_wifi_connect()
│   │   │   ├── 📄 wifi_manager_task.c      # Connects to AP, monitors and reconnects
│   │   │   └── 📁 include/
│   │   │       └── 📄 (wifi headers)       # WIFI_CONNECTED_BIT, event group declarations
│   │   │
│   │   ├── 📁 cloud_mqtt/                  # MQTT communication component
│   │   │   ├── 📄 CMakeLists.txt
│   │   │   ├── 📄 mqtt_driver.c            # esp-mqtt wrapper - init, publish, subscribe
│   │   │   ├── 📄 cloud_mqtt_task.c        # TLS connection, publishes telemetry
│   │   │   ├── 📁 certs/                   # X.509 credentials embedded at compile time
│   │   │   │   ├── 📄 AmazonRootCA1.pem    # Verifies AWS broker identity
│   │   │   │   ├── 📄 certificate.pem.crt  # Proves device identity
│   │   │   │   └── 📄 private.pem.key      # Signs TLS handshake
│   │   │   └── 📁 include/
│   │   │       └── 📄 cloud_mqtt.h         # MQTT_CONNECTED_BIT, topic defines
│   │   │
│   │   ├── 📁 ota_update/                  # OTA firmware update component
│   │   │   ├── 📄 CMakeLists.txt
│   │   │   ├── 📄 ota_task.c               # Waits on job notification, publishes status
│   │   │   ├── 📄 ota_update.c             # esp_https_ota - downloads + flashes firmware
│   │   │   ├── 📄 json_parser.c            # cJSON - extracts jobId and presigned URL
│   │   │   └── 📁 include/
│   │   │       └── 📄 (ota headers)        # JOB_NOTIFICATION_BIT, job_doc_mutex
│   │   │
│   │   └── 📁 uart/                        # UART communication component
│   │       ├── 📄 CMakeLists.txt
│   │       ├── 📄 uart2_driver.c           # Bare UART2 driver, stream buffer ISR
│   │       ├── 📄 uart_router_task.c       # Frames/validates packets, routes by type
│   │       ├── 📄 uart_tx_task.c           # TX state machine - handshake, data, ACK/retry
│   │       ├── 📄 uart_rx_task.c           # Handles incoming packets, sends ACKs
│   │       ├── 📄 crc_16.c                 # CRC16-CCITT - shared with STM32 implementation
│   │       └── 📁 include/
│   │           ├── 📄 uart_driver.h        # Driver-level declarations
│   │           ├── 📄 uart_tasks.h         # Task handles, queue declarations
│   │           ├── 📄 crc_16.h             # CRC function prototypes
│   │           └── 📄 shared_resources.h   # Shared queues, structs across UART tasks
│   │
│   ├── 📁 build/                           # Build output folder
│   ├── 📄 sdkconfig                        # ESP-IDF project configuration
│   └── 📄 CMakeLists.txt                   # Top-level build system configuration
```
