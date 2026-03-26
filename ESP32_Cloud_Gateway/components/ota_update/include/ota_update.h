#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

/**
 * @file  ota_update.h
 * @brief OTA update module declarations
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

#define JOBS_TOPIC_NOTIFY           "$aws/things/ESP32_field_01/jobs/notify"
#define JOBS_TOPIC_NOTIFY_NEXT      "$aws/things/ESP32_field_01/jobs/notify-next"
#define JOBS_TOPIC_UPDATE           "$aws/things/ESP32_field_01/jobs/%s/update"  

extern char job_document[512];
extern SemaphoreHandle_t job_doc_mutex;

extern EventGroupHandle_t ota_event_group;
#define JOB_NOTIFICATION_BIT      BIT0    // Bit set when an OTA job notification is received

// Function Prototypes
esp_err_t ota_perform_update(const char *url);
void wait_until_job_notification(void);
bool parse_job_document(const char *doc, char *job_id, size_t job_id_len, char *url, size_t url_len);

void ota_task_init(void);

#endif      // OTA_UPDATE_H