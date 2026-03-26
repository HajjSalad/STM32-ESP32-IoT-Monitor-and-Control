/**
 * @file main.c
 * @brief
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"

#include "cloud_mqtt.h"
#include "ota_update.h"
#include "uart_tasks.h"
#include "uart_driver.h"
#include "queue_push_task.h"
#include "shared_resources.h"
#include "wifi.h"

static const char *TAG = "MAIN";

QueueHandle_t tx_queue;
QueueHandle_t rx_queue;

void app_main()
{
    ESP_LOGI(TAG, "Startup...");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    uart2_init();     // Initialize UART2 for comm with STM32

    /** 
     *  System-wide Initialization
     *    nvs_flash_init() -> Non-Volatile storage to persist data across reboots
     *    esp_netif_init() -> Create an LwIP core task and initialize LwIP-related work
     *    esp_event_loop_create_default() -> Create a system Event task and 
     *               initialize an application event's callback function
     * 
     *   These are used by: Wi-Fi driver, MQTT driver, 
    */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /**
     * Priorities  (in FreeRTOS, 0 = lowest priority)
     * 
     * Priority 8  → Queue_push
     * Priority 7  → Router
     * Priority 6  → TX
     * Priority 5  → RX
     * Priority 4  → Wi-Fi
     * Priority 4  → MQTT
     * Priority 3  → OTA
     * Priority 0  → Idle task
    */

    // Create RX Queue
    tx_queue = xQueueCreate(10, sizeof(TXQueue_Item_t));
    if (tx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create tx_queue\n");
        return;
    }

    // Create TX Queue
    rx_queue = xQueueCreate(10, sizeof(RXQueue_Item_t));
    if (rx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create rx_queue\n");
        return;
    }

    // Create tasks
    queue_push_task_init();             // Priority: 4
    uart_router_task_init();            // Priority: 6
    uart_tx_task_init();                // Priority: 5
    uart_rx_task_init();                // Priority: 5
    //uart_rxtx_task_init();              // UART2 communicates with STM32
    wifi_manager_task_init();           // Wi-Fi Manager
    cloud_mqtt_task_init();             // MQTT Cloud subscriptions
    ota_task_init();                    // Listens for OTA updates 
}
