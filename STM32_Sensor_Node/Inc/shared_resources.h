#ifndef SHARED_RESOURCES_H
#define SHARED_RESOURCES_H

/**
 * @file  shared_resources.h
 * @brief Shared constants, macros, and type definitions used across all modules.
*/

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"
#include <stdio.h>

/** @brief Format for printf */
#define LOG(fmt, ...)  printf( (fmt "\n\r"), ##__VA_ARGS__)

/* --- Protocol constants — must match on both STM32 and ESP32 --- */
#define SOF_BYTE             0xAAU       // Start of frame — marks beginning of every packet
#define EOF_BYTE             0x55U       // End of frame — marks end of every packet

#define PROTOCOL_VERSION     0x01U       // Increment when protocol changes break compatibility
#define DEVICE_ID_STM32      0x01U       // Sensor node
#define DEVICE_ID_ESP32      0x02U       // Cloud gateway

/* --- Byte offsets within every UART packet frame --- */
#define SOF_POS              0U          // Start of frame
#define VER_POS              1U          // Protocol version
#define DEV_POS              2U          // Device ID
#define SEQ_POS              3U          // Sequence number
#define TYPE_POS             4U          // Packet type
#define LEN_POS              5U          // Payload length (data packets only)
#define PAYLOAD_POS          6U          // Payload start (data packets only)

// Mutex to protect shared sensor object access between sensor_write and sensor_read
extern SemaphoreHandle_t        xSensorMutex;

// Byte stream from USART1 ISR to uart_router task
extern StreamBufferHandle_t     xUartStreamBuffer;

// Logger queue — Task_read, Task_controller and Task_transmit post log messages to Task_logger
#define LOG_MSG_MAX_LEN         (128U)
#define LOG_QUEUE_DEPTH         (20U)
extern QueueHandle_t            xLogQueue;

// Sensor data queue — sensor_read posts sensor readings, controller consumes them
#define SENSOR_QUEUE_DEPTH      (20U)
extern QueueHandle_t            xSensorQueue;

// Sensor data type identifier
typedef enum {
    TEMP_SENSOR_DATA   = 0x01,      // bit 0 — temperature field is valid
    MOTION_SENSOR_DATA = 0x02,      // bit 1 — motion field is valid
} SensorDataFlags_t;

// Sensor data packet
typedef struct __attribute__((packed)) {
    uint8_t   flags;            // bitmask — indicates which fields are valid
    uint16_t  tempData;         // valid if flags & TEMP_SENSOR_DATA
    uint8_t   motionData;       // valid if flags & MOTION_SENSOR_DATA
} SensorData_t; 

extern volatile uint8_t motion_detected;  /* Motion detection: 0/1 — GPIO */

// TX and RX Queues 
#define TX_QUEUE_LENGTH         10
#define RX_QUEUE_LENGTH         10
extern QueueHandle_t            xTXQueue;
extern QueueHandle_t            xRXQueue;

/* ---  TX Side  --- */

/** Temperature reading from STM32 sensor node — scaled by 100 (e.g. 2540 = 25.40°C) */
typedef struct __attribute__((packed)) {
    uint16_t temperature;
} TempData_t;

/** Motion detection from STM32 sensor node — 1 = detected, 0 = none */
typedef struct __attribute__((packed)) {
    uint8_t motion;
} MotionData_t;

/** Status and event messages sent from STM32 to ESP32 gateway */
typedef enum {
    MSG_HEARTBEAT               = 0x01,   // STM32 alive and running
    MSG_FIRMWARE_UPDATE_OK      = 0x02,   // OTA flash succeeded
    MSG_FIRMWARE_UPDATE_FAIL    = 0x03,   // OTA flash failed
    MSG_SYSTEM_BOOT             = 0x04,   // STM32 booted or rebooted
} MessageCode_t;

/** Message packet payload — code identifies the event */
typedef struct __attribute__((packed)) {
    uint8_t code;    // MessageCode_t
} MessageData_t;

/** Identifies what type of payload is in the TX queue item */
typedef enum {
    TX_PAYLOAD_TEMP_DATA      = 0x01,
    TX_PAYLOAD_MOTION_DATA    = 0x02,
    TX_PAYLOAD_MESSAGE_DATA   = 0x03,
} TX_PayloadType_t;

/** Item placed on TX queue — holds parsed packet data for the TX task */
typedef struct {
    TX_PayloadType_t payloadType;       // Identifies active union member
    union {
        TempData_t    tempData;
        MotionData_t  motionData;
        MessageData_t messageData;
    } payload;
} TXQueue_Item_t;

/* ---  RX Side  --- */

/** Commands sent from ESP32 gateway to STM32 sensor node */
typedef enum {
    CMD_OTA_START               = 0x01,   // Begin OTA firmware update
    CMD_OTA_END                 = 0x02,   // All chunks sent, verify and reboot
    CMD_SET_TEMP_THRESHOLD      = 0x03,   // Set temperature alert limit
} CommandCode_t;

/** Command packet payload  */
typedef struct __attribute__((packed)) {
    uint8_t  code;                       // CommandCode_t - code identifies the command
    uint16_t value;                      // optional ex. threshold value, 0 if unused
} CommandData_t;

/** Firmware binary is split into chunks and sent sequentially */
#define MAX_FIRMWARE_CHUNK_SIZE      128U

typedef struct __attribute__((packed)) {
    uint8_t  data[MAX_FIRMWARE_CHUNK_SIZE];  // Chunk of firmware binary
    uint8_t  chunk_index;                    // Index of this chunk (0-based)
    uint8_t  total_chunks;                   // Total number of chunks in this update
} FirmwareData_t;

/** Identifies what type of payload is in the RX queue item */
typedef enum {
    RX_PAYLOAD_COMMAND    = 0x01,
    RX_PAYLOAD_FIRMWARE   = 0x02,
} RX_PayloadType_t;

/** Item placed on RX queue — union holds either a command or firmware chunk */
typedef struct __attribute__((packed)) {
    uint8_t seq;
    uint8_t type;
    RX_PayloadType_t payloadType;
    union {
        CommandData_t  command;
        FirmwareData_t firmware;
    } payload;
} RXQueue_Item_t;

/*  ---   Shared by TX and RX   --- */

/** Identifies the type of each UART packet — both sides must agree on these values */
typedef enum {
    PKT_HANDSHAKE_REQ    = 0x01,   // Sender requests handshake
    PKT_HANDSHAKE_ACK    = 0x02,   // Receiver acknowledges handshake
    PKT_TEMP_DATA        = 0x03,   // Temperature sensor reading
    PKT_TEMP_DATA_ACK    = 0x04,   // Acknowledge temperature packet
    PKT_MOTION_DATA      = 0x05,   // Motion detection event
    PKT_MOTION_DATA_ACK  = 0x06,   // Acknowledge motion packet
    PKT_MESSAGE_DATA     = 0x07,   // Status or event message
    PKT_MESSAGE_DATA_ACK = 0x08,   // Acknowledge message packet
    PKT_COMMAND          = 0x09,   // Control command from gateway to sensor
    PKT_COMMAND_ACK      = 0x0A,   // Acknowledge command packet
    PKT_FIRMWARE         = 0x0B,   // Firmware chunk for OTA update
    PKT_FIRMWARE_ACK     = 0x0C,   // Acknowledge firmware chunk
} UART_PacketType_t;

/** Handshake and ACK packet — no payload, used for flow control */
typedef struct __attribute__((packed)) {
    uint8_t  sof;        // Start of frame (0xAA)
    uint8_t  version;    // Protocol version
    uint8_t  device_id;  // Sender device ID
    uint8_t  seq_num;    // Transaction sequence number
    uint8_t  type;       // UART_PacketType_t
    uint8_t  crc_high;   // CRC16 high byte
    uint8_t  crc_low;    // CRC16 low byte
    uint8_t  eof;        // End of frame (0x55)
} UART_ACK_Packet_t;

/** Handshake request packet — same structure as ACK, separate type for clarity */
typedef struct __attribute__((packed)) {
    uint8_t  sof;
    uint8_t  version;
    uint8_t  device_id;
    uint8_t  seq_num;
    uint8_t  type;
    uint8_t  crc_high;
    uint8_t  crc_low;
    uint8_t  eof;
} UART_Handshake_Packet_t;

/** Maximum payload size — sized to fit the largest payload type (FirmwareData_t) */
#define MAX_PAYLOAD_SIZE     131U

/** Data packet — carries sensor readings, messages, commands, or firmware chunks */
typedef struct __attribute__((packed)) {
    uint8_t  sof;                        // Start of frame (0xAA)
    uint8_t  version;                    // Protocol version
    uint8_t  device_id;                  // Sender device ID
    uint8_t  seq_num;                    // Transaction sequence number
    uint8_t  type;                       // UART_PacketType_t
    uint8_t  length;                     // Payload length in bytes
    uint8_t  payload[MAX_PAYLOAD_SIZE];  // Payload data
    uint8_t  crc_high;                   // CRC16 high byte
    uint8_t  crc_low;                    // CRC16 low byte
    uint8_t  eof;                        // End of frame (0x55)
} UART_Data_Packet_t;

#endif // SHARED_RESOURCES_H