
// Wi-Fi

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_event.h"

#include "wifi.h"


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}

void wifi_connect()
{
    /**
     *    1. TCP/IP and Wi-Fi Stack Initialization
     *       ie Wi-Fi/LwIP Init Phase
    */

    // s1.1: Create an LwIP core task and initialize LwIP-related work
    esp_netif_init();  

    // s1.2: Create a system Event task and initialize an application event's callback function
    esp_event_loop_create_default();

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
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);

    // s3.1: Starts the Wi-Fi driver - Triggers `WIFI_EVENT_STA_START` in the event handler
    esp_wifi_start();

    // s4.1: Wi-Fi Connect Phase - Triggers the actual connection attempt using the credentials
    esp_wifi_connect();
}