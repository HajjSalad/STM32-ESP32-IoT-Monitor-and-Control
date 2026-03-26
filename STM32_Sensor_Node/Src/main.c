/**
 * @file  main.c
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

#include "uart_driver.h"
#include "shared_resources.h"
#include "tasks.h"
#include "rtc_driver.h"

SemaphoreHandle_t    xSensorMutex      = NULL;      // Mutex to protect shared sensor object access
QueueHandle_t        xSensorQueue      = NULL;      // Sensor data queue btwn sensor_read and controller
QueueHandle_t        xTXQueue          = NULL;      // UART TX queue
QueueHandle_t        xRXQueue          = NULL;      // UART RX queue
QueueHandle_t        xLogQueue         = NULL;      // Logger queue for system logger

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
    RTC_init();                 // Initialize RTC

    check_reset_cause();        // Log the cause of the last reset

    LOG("*** STM32 Sensor Node Starting ***");

    RTC_get_time();
    LOG("Time: %02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);

    RTC_get_date();
    LOG("Date: %02d/%02d/%02d", sDate.Day, sDate.Month, sDate.Year);

    // Create synchronization primitives
    xSensorMutex = xSemaphoreCreateMutex();
    configASSERT(xSensorMutex != NULL);

    xSensorQueue = xQueueCreate(SENSOR_QUEUE_DEPTH, sizeof(SensorData_t));
    configASSERT(xSensorQueue != NULL);

    xTXQueue = xQueueCreate(TX_QUEUE_LENGTH, sizeof(TXQueue_Item_t));
    configASSERT(xTXQueue != NULL);

    xRXQueue = xQueueCreate(RX_QUEUE_LENGTH, sizeof(RXQueue_Item_t));
    configASSERT(xRXQueue != NULL);

    xLogQueue = xQueueCreate(LOG_QUEUE_DEPTH, LOG_MSG_MAX_LEN);
    configASSERT(xLogQueue != NULL);

    /**
     * Priorities  (in FreeRTOS, 0 = lowest priority)
     * MAX_PRIORITIES = 16
     * 
     * Priority 14 → Timer service task
     * Priority 8  → SensorWrite
     * Priority 7  → SensorRead
     * Priority 6  → Controller
     * Priority 5  → Router task
     * Priority 4  → TX task
     * Priority 4  → RX task
     * Priority 3  → Logger
     * Priority 0  → Idle task
    */

    // Create tasks
    xRet = xTaskCreate(vTaskSensorWrite, "SensorWrite", 512, NULL, 8, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskSensorRead,  "SensorRead",  512, NULL, 7, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskController,  "Controller",  512, NULL, 6, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskRouter,       "Router",     2048, NULL, 5, &xRouterTaskHandle);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskTX,            "TX",        4096, NULL, 4, &xTXTaskHandle);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskRX,            "RX",        2048, NULL, 4, NULL);
    configASSERT(xRet == pdPASS);
    xRet = xTaskCreate(vTaskLogger,       "Logger",     1024, NULL, 3, NULL);
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