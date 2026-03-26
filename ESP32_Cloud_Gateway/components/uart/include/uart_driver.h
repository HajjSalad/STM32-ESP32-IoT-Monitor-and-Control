#ifndef UART_DRIVER_H
#define UART_DRIVER_H

/**
 * @file  uart_driver.h
 * @brief Public API for UART2 peripheral.
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_err.h"

#include "shared_resources.h"

#define UART_2_TX           17
#define UART_2_RX           16
#define UART_NUM2           UART_NUM_2
#define BUF_SIZE            1024

extern QueueHandle_t uart_2_queue;

// Function Prototype
esp_err_t uart2_init(void);

#endif  // UART_DRIVER_H

// #ifndef UART2_H
// #define UART2_H

// /**
//  * @file uart2.h
//  * @brief
// */

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "esp_system.h"

// #define UART_2_TX 17
// #define UART_2_RX 16
// #define UART_NUM2 UART_NUM_2
// #define BUF_SIZE 1024
// #define RX_BUF_SIZE 40  // Fixed buffer size for incoming data

// extern QueueHandle_t uart_2_queue;

// // Function Prototype
// esp_err_t uart2_init(void);
// //void uart_rxtx_task_init(void);

// #endif  // UART2_H