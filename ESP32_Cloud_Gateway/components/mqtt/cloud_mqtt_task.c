/**
 * @file  cloud_mqtt_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "mqtt.h"
#include "task_priorities.h"

#define WIFI_MANAGER_TASK_PERIOD_MS     (5000)
#define PUBLISH_WAIT_MS                 (20000)

static const char *TAG   = "CLOUD_MQTT";
static char topic[50]    = "sensor/data";
static int qos           = 1;

// Sensor data queue
QueueHandle_t sensor_queue;

typedef struct {
    uint32_t temperature;
    uint32_t motion;
    uint32_t sensor_id;
    uint32_t room;
} sensor_data_t;

/**
 * @brief Format sensor reading into a JSON payload string.
*/
static void format_payload(char *buf, size_t buf_len, const sensor_data_t *data)
{
    snprintf(buf, buf_len,
        "{"
        "\"temperature\":%lu,"
        "\"motion\":%lu,"
        "\"sensor_id\":\"%03lu\","
        "\"room\":\"%03lu\""
        "}",
        data->temperature,
        data->motion,
        data->sensor_id,
        data->room
    );
}

/**
 * @brief Generates random sensor data and pushes to queue every 5 seconds.
*/
static void sensor_generator_task(void *pvParameters)
{
    sensor_data_t data;

    while (1)
    {
        data.temperature = 20 + (rand() % 16);     // Random temp 20-35°C
        data.motion      = rand() % 2;              // Random motion 0 or 1
        data.sensor_id   = 101;
        data.room        = 101;

        if (xQueueSend(sensor_queue, &data, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, sensor data dropped.");
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_MANAGER_TASK_PERIOD_MS));     // Send every 5s
    }
}

static void cloud_mqtt_task(void *pvParameters)
{
    // Initialize and connect to AWS IoT Core
    if (mqtt_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "MQTT init failed, task exiting.");
        vTaskDelete(NULL);      // No need for the task then
        return;
    }

    // Wait until MQTT is connected
    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    
    ESP_LOGI(TAG, "MQTT ready, starting publish loop");

    sensor_data_t data;
    char payload[256];

    while(1) 
    {
        if (xQueueReceive(sensor_queue, &data, pdMS_TO_TICKS(PUBLISH_WAIT_MS)))
        {
            // Format the payload
            format_payload(payload, sizeof(payload), &data);
            ESP_LOGI(TAG, "Publishing: %s", payload);

            // Publish the payload
            if (mqtt_publish(topic, payload, qos) != ESP_OK)
            ESP_LOGW(TAG, "Publish failed");
        }
        else {
            ESP_LOGW(TAG, "No sensor data received in %dms", PUBLISH_WAIT_MS);
        }    
    }
}

void cloud_mqtt_task_init()
{
    sensor_queue = xQueueCreate(10, sizeof(sensor_data_t));
    if (sensor_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create sensor queue.");
        return;
    }

    xTaskCreate(sensor_generator_task, "sensor_gen_task", 2048, NULL, TASK_PRIO_MEDIUM, NULL);
    xTaskCreate(cloud_mqtt_task, "cloud_mqtt_task", 4096, NULL, TASK_PRIO_MEDIUM, NULL);
}

// esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
// {
//     switch (evt->event_id)
//     {
//     case HTTP_EVENT_ON_DATA:
//         printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
//         break;
//
//     default:
//         break;
//     }
//     return ESP_OK;
// }
//
// void post_rest_function(uint32_t *bufferSend)
// {
//     esp_http_client_config_t config_post = {
//         .url = "https://acqql8v2q7crf-ats.iot.us-east-1.amazonaws.com:8443/topics/topic1?qos=1",
//         .method = HTTP_METHOD_POST,
//         .cert_pem = root_ca,
//         .client_cert_pem = device_cert,
//         .client_key_pem = private_key,
//         .event_handler = client_event_post_handler
//     };
//    
//     esp_http_client_handle_t client = esp_http_client_init(&config_post);
//
//     char post_data[256]; // Buffer for JSON string
//    
//     for (int i = 0; i < SENSOR_NUM; i += 2) {
//         int roomIndex = i / 2;
//        
//         // Format JSON payload for one sensor reading
//         snprintf(post_data, sizeof(post_data),
//                  "{"
//                  "\"temperature\":%lu,"  // Temperature measure
//                  "\"motion\":%lu,"        // Motion measure
//                  "\"sensor_id\":\"%03d\"," // Sensor ID as string
//                  "\"room\":\"%03d\""    // Room number as string
//                  "}",
//                  bufferSend[i],     // Temperature value
//                  bufferSend[i + 1], // Motion value
//                  101 + roomIndex,   // Sensor ID
//                  101 + roomIndex);  // Room number
//
//         // Set HTTP headers and post data
//         esp_http_client_set_post_field(client, post_data, strlen(post_data));
//         esp_http_client_set_header(client, "Content-Type", "application/json");
//        
//         // Send the HTTP request
//         esp_http_client_perform(client);
//     }
//    
//     esp_http_client_cleanup(client);
// }



