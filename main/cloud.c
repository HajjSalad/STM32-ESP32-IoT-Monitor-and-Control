

#include "esp_event.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

AWS_IoT_Client client;          // MQTT client handle

// MQTT parameters
static char clientID[20] = "esp32_device_1";
static char topic[50] = "sensor/data";
static int qos = 1;

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

void aws_iot_init() {
    IoT_Error_t rc = FAILURE;
    
    // Initialize AWS IoT
    rc = aws_iot_mqtt_init(&client, AWS_IOT_MQTT_CLIENT);
    if(SUCCESS != rc) {
        printf("AWS IoT", "MQTT Init error: %d", rc);
        return;
    }
    
    // Configure MQTT client
    AWS_IoT_Client_Init_Params params = iotClientInitParamsDefault;
    params.pHostURL = data.aws_iot_endpoint.iot.endpoint_address;
    params.port = 8883;
    params.pRootCALocation = root_ca;
    params.pDeviceCertLocation = device_cert;
    params.pDevicePrivateKeyLocation = private_key;
    params.mqttCommandTimeout_ms = 20000;
    params.tlsHandshakeTimeout_ms = 10000;
    params.isSSLHostnameVerify = true;
    params.disconnectHandler = NULL;
    params.clientID = clientID;
    params.clientIDLen = (uint16_t) strlen(clientID);
    
    rc = aws_iot_mqtt_connect(&client, &params);
    if(SUCCESS != rc) {
        ESP_LOGE("AWS IoT", "MQTT Connect error: %d", rc);
        return;
    }
    
    ESP_LOGI("AWS IoT", "Connected to AWS IoT Core");
}

void publish_sensor_data(uint32_t *bufferSend) {
    IoT_Error_t rc = FAILURE;
    char payload[256];
    
    for (int i = 0; i < SENSOR_NUM; i += 2) {
        int roomIndex = i / 2;
        
        snprintf(payload, sizeof(payload),
                "{"
                "\"temperature\":%lu,"
                "\"motion\":%lu,"
                "\"sensor_id\":\"%03d\","
                "\"room\":\"%03d\""
                "}",
                bufferSend[i],     // Temperature value
                bufferSend[i + 1], // Motion value
                101 + roomIndex,   // Sensor ID
                101 + roomIndex);  // Room number

        rc = aws_iot_mqtt_publish(&client, topic, strlen(topic), 
                                 (uint8_t *)payload, strlen(payload), 
                                 qos, NULL, NULL);
        
        if(SUCCESS != rc) {
            printf("AWS IoT", "Publish error: %d", rc);
        } else {
            printf("AWS IoT", "Published: %s", payload);
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}