#ifndef MQTT_H
#define MQTT_H

/**
 * @file mqtt.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#define AWS_ENDPOINT    "acqql8v2q7crf-ats.iot.us-east-1.amazonaws.com"

#define clientID  "esp32_field_01"

extern EventGroupHandle_t mqtt_event_group;
#define MQTT_CONNECTED_BIT      BIT0    // Bit set when Wi-Fi is connected

// Function Prototype
esp_err_t mqtt_init(void);
esp_err_t mqtt_publish(const char *topic, const char *payload, int qos);
//esp_err_t mqtt_subscribe();
bool mqtt_is_connected(void);
void cloud_mqtt_task_init();

#endif  // MQTT_H