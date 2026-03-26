/**
 * @file  ota_task.c
 * @brief OTA update task - handles IoT Job notifications and triggers OTA update.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "stdio.h"

#include "cloud_mqtt.h"
#include "ota_update.h"
#include "task_priorities.h"

static const char *TAG = "OTA_TASK";

// Event group used to signal Job notification
EventGroupHandle_t ota_event_group;

char job_document[512];
SemaphoreHandle_t job_doc_mutex;

/**
 * @brief Publishes job status update to AWS IoT Jobs
 * 
 * @param job_id    Job ID extracted from the job document
 * @param status    Job status string
 */
void publish_job_status(const char *job_id, const char *status)
{
    char topic[128];
    char payload[64];

    snprintf(topic,   sizeof(topic),  JOBS_TOPIC_UPDATE, job_id);
    snprintf(payload, sizeof(payload), "{\"status\":\"%s\"}", status);

    if (mqtt_publish_enqueue(topic, payload, strlen(payload), 1, 0, false) != ESP_OK){
        ESP_LOGW(TAG, "Failed to publish job status: %s", status);
    } else {
        ESP_LOGI(TAG, "Job [%s] status published: %s", job_id, status);
    }
}

/**
 * @brief  Handles an OTA job - publishes status updates and triggers firmware update
 * 
 * @param job_id    Job ID extracted from the job document
 * @param url       S3 presigned URL extracted from the job document
*/
static void ota_job_handler(const char *job_id, const char *url)
{
    ESP_LOGI(TAG, "OTA job received - job_id: %s", job_id);

    // Publish "IN_PROGRESS" to AWS
    publish_job_status(job_id, "IN_PROGRESS");

    // Perform update
    esp_err_t ret = ota_perform_update(url);
    
    if (ret == ESP_OK) {
        // Device reboots inside ota_perform_update() on success - this line never reached
        publish_job_status(job_id, "SUCCEEDED");
    } else {
        ESP_LOGE(TAG, "OTA update failed");
        publish_job_status(job_id, "FAILED");
    }
}

/**
 * @brief Subscribes to AWS IoT Jobs topic and blocks waiting for job notifications
*/
static void ota_task(void *pvParameters)
{
    ESP_LOGI(TAG, "In OTA_TASK");

    // @ref mqtt_client.h line 374
    const esp_mqtt_topic_t topics[] = {
        { .filter = JOBS_TOPIC_NOTIFY,      .qos = 1 },      // Notified when a job is pending
        { .filter = JOBS_TOPIC_NOTIFY_NEXT, .qos = 1 },      // Receives full job document with jobId and URL
    };

    // Wait untill MQTT is connected
    mqtt_wait_until_connected();

    // Subscribe to AWS IoT Jobs topics
    uint8_t retries = 0;
    if (mqtt_subscribe_multiple(topics, sizeof(topics) / sizeof(topics[0])) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to Jobs topics");
        retries++;
        if (retries >= 5) {
            ESP_LOGE(TAG, "Subscribe failed after 5 attempts - deleting OTA task");
            vTaskDelete(NULL);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(5000));        // wait 5 secs before retry
    }
    ESP_LOGI(TAG, "Subscribed to Jobs topics successfully");

    while(1) 
    {
        // Block waiting for job notification
        wait_until_job_notification();        // BIT set by MQTT_EVENT_DATA in mqtt_event_handler

        // Read job document - use mutex for safe accessing
        xSemaphoreTake(job_doc_mutex, portMAX_DELAY);
        char local_doc[512];
        strncpy(local_doc, job_document, sizeof(local_doc) - 1);
        local_doc[sizeof(local_doc) - 1] = '\0';            // null-terminate
        xSemaphoreGive(job_doc_mutex);

        // Parse job document JSON to extract and copy job_id and url
        char job_id[64];
        char url[256];
        if (!parse_job_document(local_doc, job_id, sizeof(job_id), url, sizeof(url))) {
            ESP_LOGE(TAG, "Failed to pardejob document - skipping");
            publish_job_status(job_id, "FAILED");
            continue;       // Back to waiting for job notification
        }
        
        // Handle the ota job
        ota_job_handler(job_id, url);
    }  
}

void ota_task_init(void)
{
    ota_event_group = xEventGroupCreate();
    xTaskCreate(ota_task, "ota_task", 4096, NULL, TASK_PRIO_MEDIUM, NULL);
}