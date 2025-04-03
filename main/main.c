/*
    main.c - Entry point and main flow of the programs
*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/uart_struct.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "demo.h"
#include "wrapper.h"
#include "priorities.h"
#include "control_task.h"
#include "my_data.h"

#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define UART_0_TX              1       // TX pin
#define UART_0_RX              3       // RX pin
#define RX_BUF_SIZE            256
#define TX_BUF_SIZE            256
#define UART_BAUD_RATE         115200
#define UART_TASK_STACK_SIZE   3072

extern const uint8_t cert_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t cert_end[]   asm("_binary_AmazonRootCA1_pem_end");
extern const uint8_t certificate_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_end[]   asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_end[]   asm("_binary_private_pem_key_end");

// mutex for concurrent access of UART
SemaphoreHandle_t UARTSemaphore; 

// Initialize UART
void uart_init(void) {
    // Configure UART Parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_0, &uart_config);  
    uart_set_pin(UART_NUM_0, UART_0_TX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); 
    uart_driver_install(UART_NUM_0, RX_BUF_SIZE, TX_BUF_SIZE, 0, NULL, 0);
}

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

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation 					s1.1
    esp_event_loop_create_default();     // event loop 			                s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station 	                    s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); // 					                    s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS
            }
        };
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
}

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}

void post_rest_function(uint32_t *bufferSend)
{
    esp_http_client_config_t config_post = {
        .url = "https://acqql8v2q7crf-ats.iot.us-east-1.amazonaws.com:8443/topics/topic1?qos=1",
        .method = HTTP_METHOD_POST,
        .cert_pem = (const char *)cert_start,
        .client_cert_pem = (const char *)certificate_start,
        .client_key_pem = (const char *)private_start,
        .event_handler = client_event_post_handler};
    
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char post_data[256]; // Buffer for JSON string
    
    for (int i = 0; i < SENSOR_NUM; i += 2) {
        int roomIndex = i / 2;
        
        // Format JSON payload for one sensor reading
        snprintf(post_data, sizeof(post_data),
                 "{"
                 "\"temperature\":%lu,"  // Temperature measure
                 "\"motion\":%lu,"        // Motion measure
                 "\"sensor_id\":\"%03d\"," // Sensor ID as string
                 "\"room\":\"%03d\""    // Room number as string
                 "}",
                 bufferSend[i],     // Temperature value
                 bufferSend[i + 1], // Motion value
                 101 + roomIndex,   // Sensor ID
                 101 + roomIndex);  // Room number

        // Set HTTP headers and post data
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_http_client_set_header(client, "Content-Type", "application/json");
        
        // Send the HTTP request
        esp_http_client_perform(client);
    }
    
    esp_http_client_cleanup(client);
}

// uart Task - Print every 2mins to indicate the program is alive
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
    // Create semaphore
    UARTSemaphore = xSemaphoreCreateMutex();

    demo_init();
    nvs_flash_init();
    wifi_connection();

    vTaskDelay(5000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");

    //post_rest_function();
    
    // Create the tasks
    uartTaskInit();         
    controlTaskInit();  
}