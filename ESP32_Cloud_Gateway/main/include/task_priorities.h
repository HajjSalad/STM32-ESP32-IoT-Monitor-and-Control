#ifndef TASK_PRIORITIES_H
#define TASK_PRIORITIES_H

/**
 * @file uart2.h
 * @brief
*/

#include "freertos/FreeRTOS.h"

// Largest number = higher priority
#define TASK_PRIO_IDLE           0
#define TASK_PRIO_LOW            1      // Filtering, sensor maths, 
#define TASK_PRIO_MEDIUM         2      // Cloud TX, OTA update, 
#define TASK_PRIO_HIGH           3      // Wi-Fi Manager, 
#define TASK_PRIO_CRITICAL       4      // UART RX, 
#define TASK_PRIO_MAX            (configMAX_PRIORITIES - 1)

#endif  // TASK_PRIORITIES_H
