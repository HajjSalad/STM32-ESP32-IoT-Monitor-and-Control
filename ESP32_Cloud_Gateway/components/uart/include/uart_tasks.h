#ifndef UART_TX_TASK_H_
#define UART_TX_TASK_H_

/**
 * @file  uart_tx_task.h
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#include "shared_resources.h"

// Largest number = higher priority
#define TASK_PRIO_6           6
#define TASK_PRIO_5           5   
#define TASK_PRIO_4           4     
#define TASK_PRIO_3           3        
#define TASK_PRIO_MAX        (configMAX_PRIORITIES - 1)

// ESP32 side - high bit 1
#define SEQ_ESP32_BASE  0x80                // seq: 0x80, 0x81, 0x82 ... 0xFF

extern TaskHandle_t xTXTaskHandle;

typedef enum {
    STATE_SEND_HANDSHAKE,
    STATE_WAIT_HANDSHAKE_ACK,
    STATE_SEND_DATA,
    STATE_WAIT_DATA_ACK,
    STATE_IDLE
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

// Function Prototypes
void uart_tx_task_init(void);
void uart_rx_task_init(void);
void uart_router_task_init(void);

#endif      // UART_TX_TASK_H_
