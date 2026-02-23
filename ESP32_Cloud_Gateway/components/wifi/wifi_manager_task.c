/**
 * @file  wifi_manager_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_wifi.h"

#include "string.h"
#include "stdio.h"

#include "wifi.h"
#include "task_priorities.h"

#define WIFI_MANAGER_TASK_PERIOD_MS     (5000U)

static const char *TAG = "WIFI_MGR";

static void wifi_manager_task(void *pvParameters)
{
    wifi_init();
    wifi_start();

    while(1) 
    {
        if (!wifi_is_connected())
        {
            ESP_LOGW(TAG, "WiFi not connected. Reconnecting...");
            esp_wifi_connect();
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_MANAGER_TASK_PERIOD_MS));        // check every given period
    }
}

void wifi_manager_task_init(void)
{
    xTaskCreate(wifi_manager_task, "wifi_manager_task", 4096, NULL, TASK_PRIO_HIGH, NULL);
}