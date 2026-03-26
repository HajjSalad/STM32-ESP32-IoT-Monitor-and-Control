#ifndef QUEUE_PUSH_TASK_H_
#define QUEUE_PUSH_TASK_H_

/**
 * @file  queue_push_task.h
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define QUEUE_PUSH_TASK_PERIOD_MS       5000        // Runs every 5 secs

// Function Prototypes
void queue_push_task_init(void);

#endif      // QUEUE_PUSH_TASK_H_