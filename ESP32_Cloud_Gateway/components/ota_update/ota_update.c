/**
 * @file  ota_update.c
 * @brief Handles firmware download from S3 and flashing
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"

#include "ota_update.h"

static const char *TAG = "OTA_UPDATE";

// Root CA certificate for verifying S3 HTTPS connection
extern const uint8_t s3_root_ca[] asm("_binary_AmazonRootCA1_pem_start");

/**
 * @brief HTTP event handler for OTA download - logs connection lifecycle events
*/
static esp_err_t _http_event_handler(esp_http_client_event_t *event)
{
    switch (event->event_id) {
    case HTTP_EVENT_ERROR:          ESP_LOGD(TAG, "HTTP_EVENT_ERROR");                                                      break;
    case HTTP_EVENT_ON_CONNECTED:   ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");                                               break;
    case HTTP_EVENT_HEADER_SENT:    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");                                                break;
    case HTTP_EVENT_ON_HEADER:      ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", event->header_key, event->header_value); break;
    case HTTP_EVENT_ON_DATA:        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", event->data_len);                           break;
    case HTTP_EVENT_ON_FINISH:      ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");                                                  break;
    case HTTP_EVENT_DISCONNECTED:   ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");                                               break;
    case HTTP_EVENT_REDIRECT:       ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");                                                   break;
    }
    return ESP_OK;
}

/**
 * @brief Downloads firmware from S3 presigned URL and flashes to inactive OTA partition
 *        Reboots on success. Returns ESP_FAIL on failure.
 * 
 * @param  url  S3 presigned URL pointing to the firmware binary
 * @return      ESP_OK on success, ESP_FAIL on failure 
*/
esp_err_t ota_perform_update(const char *url)
{
    // 1. Configure HTTP client with S3 URL and root CA for TLS verification
    esp_http_client_config_t config = {
        .url                = url,
        .cert_pem           = (char *)s3_root_ca,           // Verifies S3 endpoint authenticity
        .event_handler      = _http_event_handler,          // Logs HTTP lifecycle events
        .keep_alive_enable  = true,                         // Maintains connection during download
    };

    // 2. Wrap HTTP config in OTA config
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    // 3. Download firmware from S3 and flash to inactive OTA partition
    ESP_LOGI(TAG, "Dowmloading update from %s", url);
    esp_err_t ret = esp_https_ota(&ota_config);

    // 4a. On success - reboot to boot into new firmware
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeded, Rebooting...");
        esp_restart();          
    }

    // 4b. On failure - log error and return
    ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    return ESP_FAIL;
}

/**
 * @brief Blocks the calling task until an OTA job notification is received
*/
void wait_until_job_notification(void)
{
    xEventGroupWaitBits(
        ota_event_group, 
        JOB_NOTIFICATION_BIT,
        pdFALSE,                // Do not clear bit on exit
        pdTRUE,                 // Wait for all bits - only one here tho
        portMAX_DELAY           // Wait indefinitely
    );
}