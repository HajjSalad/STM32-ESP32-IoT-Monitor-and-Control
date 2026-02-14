
// Entry point to the program
// Create Tasks

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "uart.h"
#include "wifi.h"
#include "demo.h"
#include "cloud.h"
#include "wrapper.h"
#include "priorities.h"
#include "control_task.h"

#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"

#define UART_TASK_STACK_SIZE   1024

extern SemaphoreHandle_t UARTSemaphore;

// Task 1: Print every 2mins to check alive
static void uartTask(void *arg) 
{
    while(1) {
        if (xSemaphoreTake(UARTSemaphore, portMAX_DELAY) == pdTRUE) {
            printf("UART Task Running...\n\r");
            xSemaphoreGive(UARTSemaphore);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Create Task 1
uint32_t uartTaskInit() {
    if (xTaskCreate(uartTask, "uart_task", UART_TASK_STACK_SIZE * 2, NULL, PRIORITY_UART_TASK, NULL)) {
        printf("UART Task created..!\n\r");
        return 1;
    } else {
        printf("UART Task NOT created..!\n\r");
        return 0;
    }
}

void app_main(void)
{
    uart_init();            // Init uart
    demo_init();            // Check if C++ working
    nvs_flash_init();
    wifi_connection();      // Wi-Fi connect

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");
    
    // Create the Tasks 1 & 2
    uartTaskInit();             // Alive Task     
    controlTaskInit();          // Control Task
}