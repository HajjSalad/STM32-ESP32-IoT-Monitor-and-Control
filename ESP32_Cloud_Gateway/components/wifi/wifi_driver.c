/**
 * @file wifi_driver.c
 * @brief Wi-Fi driver module for ESP32.
 * 
 * Handles Wi-Fi initialization, connection, status monitoring, 
 * and provides API for other tasks to check/wait for connectivity.
 * 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_event.h"

#include "wifi.h"

static const char *TAG = "WIFI";

// Event group used to signal Wi-Fi connection status
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT      BIT0    // Bit set when Wi-Fi is connected          

/**
 * @brief Event handler for Wi-Fi and IP events
 * 
 * @param event_handler_arg Not used
 * @param event_base        Event base (WIFI_EVENT/IP_EVENT)
 * @param event_id          Event ID
 * @param event_data        Event-specific data
*/
static void wifi_event_handler(void *event_handler_arg, 
                               esp_event_base_t event_base, 
                               int32_t event_id, 
                               void *event_data)
{
    switch (event_id) {

    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "WiFi starting...");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "WiFi connected AP");
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "WiFi disconnected");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;

    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Got IP address");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;

    default:
        break;
    }
}

/**
 * @brief Initialize Wi-Fi subsystem (driver, event loop, interface)
 *        Does not start connection yet.
 * 
 * Adapted from: ESP-IDF -> API Guides -> Wi-Fi Driver
*/
void wifi_init(void)
{
    // Create the EventGroup
    wifi_event_group = xEventGroupCreate();

    /**
     *    1. TCP/IP and Wi-Fi Stack Initialization
     *       ie Wi-Fi/LwIP Init Phase
    */

    // s1.1: Create an LwIP core task and initialize LwIP-related work
    //esp_netif_init();   -> Initialized in app_main

    // s1.2: Create a system Event task and initialize an application event's callback function
    //esp_event_loop_create_default();    -> initialized in app_main

    // s1.3: Create default network interface instance (biding station or AP) - sta used in this case
    esp_netif_create_default_wifi_sta();

    // s1.4: Create the Wi-Fi driver task and initialize the Wi-Fi driver with default settings
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);

    /** 
     *    2. Wi-Fi Configuration Phase
    */

    /** 
     *  s2.1 Register the wifi_event_hanlder function with the event loop
     *      ESP_EVENT_ANY_ID    - listens for any Wi-Fi event (connect, disconnect, start etc)
     *      IP_EVENT_STA_GOT_IP - listens for IP event when DHCP assigns an IP address
    */
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS
        }
    };
    // Set the credentials the Wi-Fi driver will use to connect
    // Also writes credentials to NVS flash for persistence across reboots
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration); 
}

/**
 * @brief Start Wi-Fi driver and connect to the configured AP
*/
void wifi_start(void) 
{
    // s3.1: Starts the Wi-Fi driver - Triggers `WIFI_EVENT_STA_START` in the event handler
    esp_wifi_start();

    // s4.1: Wi-Fi Connect Phase - Triggers the actual connection attempt using the credentials
    esp_wifi_connect();
}

/**
 * @brief Stop Wi-Fi driver and clear connection bit
*/
void wifi_stop(void)
{
    // Stop the Wi-Fi driver
    esp_wifi_stop();
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
}

/**
 * @brief Check if Wi-Fi is currently connected
 * @return true if connected, false otherwise
*/
bool wifi_is_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}


/**
 * @brief Block the calling task until Wi-Fi is connected
 * 
 * Used by other tasks to wait for wifi to connect
*/
void wifi_wait_until_connected(void)
{
    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,                  // Do not clear bit on exit  
        pdTRUE,                   // Wait for all bits (only one bit here)  
        portMAX_DELAY             // Wait indefinitely  
    );
}
