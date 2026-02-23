/**
 * @file main.c
 * @brief Main entry point for the STM32 Sensor Node application.
 * 
 * Initializes peripherals, creates FreeRTOS resources (mutexes, queues, 
 * stream buffers), spawns all application tasks, and starts the scheduler.
*/

#include <stdint.h>

#include "stm32f446xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"

#include "uart.h"
#include "shared_resources.h"
#include "task_logger.h"
#include "task_transmit.h"
#include "task_controller.h"
#include "task_sensor_write.h"
#include "task_sensor_read.h"

#define STACK_SIZE_WORDS       (1024U)

// Global resource handles
SemaphoreHandle_t    xSensorMutex      = NULL;
QueueHandle_t        xLogQueue         = NULL;
QueueHandle_t        xSensorQueue      = NULL;
StreamBufferHandle_t xStreamBuffer     = NULL;

// Local function prototypes
static void check_reset_cause(void);

/**
 * @brief FreeRTOS stack overflow hook.
 * 
 * Called automatically when stack overflow is detected. 
 * Logs the offending task name and halts the system.
*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;                // Suppress unused parameter warning
    uart2_write('!');           // Indicate stack overflow error
    while(1) {}
}

/**
 * @brief Application entry point.
 * 
 * Initializes UART peripherals, creates FreeRTOS synchronization
 * primitives and tasks, then starts the scheduler.
*/
int main(void) 
{
    BaseType_t xRet = pdFALSE;

    uart2_init();               // Initialize UART2 for logging
    uart1_init();               // Initialize UART1 for ESP32 communication

    check_reset_cause();        // Log the cause of the last reset

    LOG("*** STM32 Sensor Node Starting ***");

    // Create synchronization primitives
    xSensorMutex = xSemaphoreCreateMutex();
    configASSERT(xSensorMutex != NULL);

    xLogQueue = xQueueCreate(LOG_QUEUE_DEPTH, LOG_MSG_MAX_LEN);
    configASSERT(xLogQueue != NULL);

    xSensorQueue = xQueueCreate(SENSOR_QUEUE_DEPTH, sizeof(SensorData_t));
    configASSERT(xSensorQueue != NULL);

    xStreamBuffer = xStreamBufferCreate(STREAM_BUFFER_SIZE, sizeof(TransmitData_t));
    configASSERT(xStreamBuffer != NULL);

    // Create tasks
    xRet = xTaskCreate(vTaskSensorWrite, "SensorWrite", STACK_SIZE_WORDS, NULL, 5, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskSensorRead,  "SensorRead",  STACK_SIZE_WORDS, NULL, 4, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskController,  "Controller",  STACK_SIZE_WORDS, NULL, 3, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskTransmit,    "Transmit",    STACK_SIZE_WORDS, NULL, 2, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskLogger,      "Logger",      STACK_SIZE_WORDS, NULL, 1, NULL);
    configASSERT(xRet == pdPASS);

    LOG("Tasks created. Free heap: %u bytes", xPortGetFreeHeapSize());
    LOG("Starting scheduler...");

    vTaskStartScheduler();  

    // Should never reach here - halt if scheduler exits
    LOG("Scheduler exited unexpectedly!");
    while (1) {}
}

/**
 * @brief Check reset cause and log it over UART.
 * Must be called before any other initialization to ensure accurate logging of reset causes.
*/
static void check_reset_cause(void) 
{
    uint32_t cause = RCC->CSR;
    RCC->CSR |= RCC_CSR_RMVF;           // Clear reset flags

    if (cause & RCC_CSR_IWDGRSTF) { LOG("Reset: Watchdog"); }
    if (cause & RCC_CSR_SFTRSTF)  { LOG("Reset: Software"); }
    if (cause & RCC_CSR_PORRSTF)  { LOG("Reset: Power-On"); }
    if (cause & RCC_CSR_PINRSTF)  { LOG("Reset: External Pin"); }
}