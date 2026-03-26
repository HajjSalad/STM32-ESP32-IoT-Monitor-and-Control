#ifndef MQTT_H
#define MQTT_H

/**
 * @file mqtt.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "mqtt_client.h"

#define AWS_ENDPOINT    "mqtts://acqql8v2q7crf-ats.iot.us-east-1.amazonaws.com:8883"

#define clientID  "esp32_field_01"

extern EventGroupHandle_t mqtt_event_group;
#define MQTT_CONNECTED_BIT      BIT0    // Bit set when MQTT is connected

// Function Prototype
esp_err_t mqtt_init(void);
esp_err_t mqtt_start(void);
esp_err_t mqtt_stop(void);
bool mqtt_is_connected(void);
void mqtt_wait_until_connected(void);
esp_err_t mqtt_publish_enqueue(const char *topic, 
                               const char *payload, 
                               size_t len, 
                               int qos, 
                               int retain, 
                               bool store);
esp_err_t mqtt_subscribe_single(const char *topic, int qos);
esp_err_t mqtt_subscribe_multiple(const esp_mqtt_topic_t *topic_list, size_t size);

void cloud_mqtt_task_init();

#endif  // MQTT_H