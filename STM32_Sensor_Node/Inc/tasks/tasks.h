#ifndef TASKS_H
#define TASKS_H

/**
 * @file  tasks.h
 * @brief Tasks interface.
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "shared_resources.h"

#include <stdio.h>

// Macros for consistent log formatting
#define LOG_SENSOR_DATA(msg, taskname, action, temp, motion) \
    snprintf(msg, sizeof(msg), "[%-12s] %-18s Temp: %3u  Motion: %u", \
            taskname, action, (unsigned int)(temp), (unsigned int)(motion))

// Macro for logging transmit data with timestamp
#define LOG_TRANSMIT_DATA(msg,taskname, action, temp, motion, ts) \
    snprintf(msg, sizeof(msg), "[%-12s] %-18s Temp: %3u  Motion: %u  Timestamp: %lu", \
             taskname, action, (unsigned int)(temp), (unsigned int)(motion), (unsigned long)(ts))

// STM32 side - high bit 0
#define SEQ_STM32_BASE  0x00                // seq: 0x00, 0x01, 0x02 ... 0x7F

#define UART_RX_BUFFER_SIZE         256

extern TaskHandle_t  xSensorSampleHandle;
extern TaskHandle_t  xSensorReadHandle;
extern TaskHandle_t  xRouterTaskHandle;
extern TaskHandle_t  xTXTaskHandle;
extern TimerHandle_t xSampleTimer;

extern volatile uint8_t  rxBuffer[UART_RX_BUFFER_SIZE];
extern volatile uint16_t rxHead;
extern volatile uint16_t rxTail;

// IWDG Task Alive flags
extern volatile uint8_t task1_alive;
extern volatile uint8_t task2_alive;
extern volatile uint8_t task3_alive;
extern volatile uint8_t task4_alive;
extern volatile uint8_t task5_alive;
extern volatile uint8_t task6_alive;
extern volatile uint8_t task7_alive;

typedef enum {
    STATE_SEND_HANDSHAKE,
    STATE_WAIT_HANDSHAKE_ACK,
    STATE_SEND_DATA,
    STATE_WAIT_DATA_ACK,
    STATE_IDLE,
} UART_TX_State_t;

typedef struct {
    UART_TX_State_t state;
    uint8_t         seq;
    uint8_t         retry_count;
} UART_TX_Context;

typedef enum {
    TX_RESULT_PENDING = 0U,
    TX_RESULT_SUCCESS = 1U,
    TX_RESULT_FAILED  = 2U,
} TX_Result_t;

typedef struct {
    uint8_t           seq;
    TX_PayloadType_t  payloadType;
    uint8_t           retries;
    uint32_t          start_tick;
    uint32_t          end_tick;
    TX_Result_t       result;
} TX_Transaction_t;

void vTaskSensorSample(void *pvParameters);
void vSampleTimerCallback(TimerHandle_t xTimer);
void vTaskSensorRead(void *pvParameters);
void vTaskController(void *pvParameters);
void vTaskRouter(void *pvParameters);
void vTaskTX(void *pvParameters);
void vTaskRX(void *pvParameters);
void vTaskLogger(void *pvParameters);
void vTaskWatchdogMonitor(void *pvParameters);

#endif // TASKS_H 