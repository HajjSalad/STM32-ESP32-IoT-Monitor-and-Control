/**
 * @file  json_parser.c
 * @brief 
*/

#include "esp_log.h"
#include "cJSON.h"
#include "string.h"
#include "stdbool.h"

#include "ota_update.h"

static const char *TAG = "JSON_PARSER";
/*

What JSON looks like:
{
    "key": "value"
}

{
    "name":     "ESP32_field_01",       
    "version":  1,                      
    "voltage":  3.14,                   
    "active":   true,                   
    "nickname": null,                   

    "address": {                        
        "street": "123 Main St",
        "city":   "Barrie"
    },

    "sensors": [                        
        "TMP117",
        "PIR",
        "BME280"
    ],

    "readings": [                       
        { "type": "temperature", "value": 23.5 },
        { "type": "motion",      "value": 1    }
    ]
}

Value types are: string, integer, float, boolean, null, nested object `{}`, array `[]`, array of objects.

OTA job document looks like:
{
    "jobId": "ota-job-001",
    "jobDocument": {
        "url":     "https://esp32-firmware-ota.s3....",
        "version": "1.1"
    }
}
*/

/**
 * 
 * Resources:
 *   - https://www.youtube.com/watch?v=FBpgdSjJ6nQ
 *   - https://github.com/Barenboim/json-parser
 *   - https://github.com/lloyd/yajl
 *   - https://github.com/kgabis/parson
 * 
 * Steps to parse a JSON document using cJSON:
 *  1. Parse the raw JSON string into a cJSON object tree (cJSON_Parse)
 *  2. Validate the root object was created successfully (null check)
 *  3. Extract the target field from the tree (cJSON_GetObjectItem)
 *  4. Validate the extracted field exists and is the expected type (cJSON_IsString, cJSON_IsObject etc.)
 *  5. Copy the extracted value into your output buffer (strncpy + manual null termination)
 *  6. Free the cJSON object tree (cJSON_Delete) — always, regardless of success or failure
*/
bool parse_job_document(const char *doc, char *job_id, size_t job_id_len, char *url, size_t url_len)
{
    bool success = false;

    // Parse the JSON string into a cJSON object tree
    // cJSON_Parse allocates the entire tree on the heap —
    // cJSON_Delete(root) must always be called to free it.
    cJSON *root = cJSON_Parse(doc);
    if (root == NULL) {
        // Parsing fails if doc is NULL, empty, or malformed JSON
        ESP_LOGE(TAG, "Failed to parse job document JSON");
        return false;
    }

    // value of `jobId` key is a string
    // Extract jobId — walks the tree from root to find the node with key "jobId"
    // cJSON_GetObjectItem returns NULL if the key does not exist
    cJSON *job_id_obj = cJSON_GetObjectItem(root, "jobId");
    if (!cJSON_IsString(job_id_obj)) {
        // Fails if jobId is missing, NULL, or not a string type
        // cJSON_IsString returns false for NULL — so no need to check specificly for NULL 
        ESP_LOGE(TAG, "jobId not found or not a string");
        goto cleanup;
    }

    // Extract jobDocument — nested object one level below root
    // cJSON_GetObjectItem walks from root to find the node with key "jobDocument"
    cJSON *job_doc = cJSON_GetObjectItem(root, "jobDocument");
    if (!cJSON_IsObject(job_doc)) {
        // Fails if jobDocument is missing, NULL, or not an object type
        // cJSON_IsObject returns false for NULL — handles both cases in one check
        ESP_LOGE(TAG, "jobDocument not found");
        goto cleanup;
    }

    // Extract url — walks from job_doc (not root) to find the node with key "url"
    // job_doc is the entry point since url is nested inside jobDocument
    cJSON *url_obj = cJSON_GetObjectItem(job_doc, "url");
    if (!cJSON_IsString(url_obj)) {
        // Fails if url is missing, NULL, or not a string type
        // cJSON_IsString returns false for NULL —
        ESP_LOGE(TAG, "url not found or not a string");
        goto cleanup;
    }

    // Copy extracted values into output buffers
    strncpy(job_id, job_id_obj->valuestring, job_id_len - 1);
    job_id[job_id_len - 1] = '\0';

    strncpy(url, url_obj->valuestring, url_len - 1);
    url[url_len - 1] = '\0';

    ESP_LOGI(TAG, "Job ID : %s", job_id);
    ESP_LOGI(TAG, "OTA URL: %s", url);

    success = true;

cleanup:
    cJSON_Delete(root);     // always free the cJSON tree
    return success;
}