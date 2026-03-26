/**
 * @file  uart_router_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>

#include "crc_16.h"
#include "uart_driver.h"
#include "uart_tasks.h"
#include "shared_resources.h"

#define ROUTER_BUFFER_SIZE     64

static const char *TAG = "ROUTER";

static void process_rx(const uint8_t *buf, uint8_t len)
{
    if (buf[SOF_POS]  != SOF_BYTE) return;         // Validate SoF
    if (buf[len - 1U] != EOF_BYTE) return;         // Validate EoF

    uint8_t crc_high    = buf[len - 3U];
    uint8_t crc_low     = buf[len - 2U];

    // Validate CRC
    uint16_t received_crc = ((uint16_t)(crc_high << 8U) | ((uint16_t)crc_low));
    uint16_t computed_crc = compute_crc(buf, len - 3U);
    
    if (received_crc != computed_crc) {
        printf("CRC FAIL - packet discarded\n");
        printf("Received CRC: 0x%02X 0x%02X\n", crc_high, crc_low);
        printf("Computed CRC: 0x%02X 0x%02X\n", (computed_crc >> 8), (computed_crc & 0x00FF));
        return;
    }

    uint8_t sof         = buf[SOF_POS];
    uint8_t ver         = buf[VER_POS];
    uint8_t dev         = buf[DEV_POS];
    uint8_t seq         = buf[SEQ_POS];
    uint8_t type        = buf[TYPE_POS];
    uint8_t eof         = buf[len - 1U];

    switch(type) 
    {
        // Route to RX 
        case PKT_HANDSHAKE_REQ:
        {
            printf("\n--- Starting new RX cycle ---\n");
            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_HANDSHAKE_REQ", sof, ver, dev, seq, type, crc_high, crc_low, eof);

            RXQueue_Item_t item = {
                .seq  = seq,
                .type = type,
            };
            xQueueSend(rx_queue, &item, 0);
            break;
        }
        // Route to TX 
        case PKT_HANDSHAKE_ACK:
        {
            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_HANDSHAKE_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);

            xTaskNotify(xTXTaskHandle, PKT_HANDSHAKE_ACK, eSetValueWithOverwrite);
            break;
        }
        // Route to RX
        case PKT_TEMP_DATA:
        {
            uint8_t payload_len = buf[LEN_POS];

            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_TEMP_DATA", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            printf("[0x%02X] %-20s : LEN:0x%02X DATA:", seq, "INFO_RX_TEMP_DATA", payload_len);
            for (uint8_t i = 0U; i < payload_len; i++) {
                printf("0x%02X ", buf[PAYLOAD_POS + i]);
            }
            printf("\n");

            // Send info to the queue
            RXQueue_Item_t item = {
                .seq         = seq,
                .type        = type,
                .payloadType = RX_PAYLOAD_TEMP_DATA,
            };
            memcpy(&item.payload.tempData, &buf[PAYLOAD_POS], payload_len);
            xQueueSend(rx_queue, &item, 0);
            break;
        }
        case PKT_MOTION_DATA:
        {
            uint8_t payload_len = buf[LEN_POS];

            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_MOTION_DATA", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            printf("[0x%02X] %-20s : LEN:0x%02X DATA:", seq, "INFO_RX_MOTION_DATA", payload_len);
            for (uint8_t i = 0U; i < payload_len; i++) {
                printf("0x%02X ", buf[PAYLOAD_POS + i]);
            }
            printf("\n");

            // Send info to the queue
            RXQueue_Item_t item = {
                .seq         = seq,
                .type        = type,
                .payloadType = RX_PAYLOAD_MOTION_DATA,
            };
            memcpy(&item.payload.motionData, &buf[PAYLOAD_POS], payload_len);
            xQueueSend(rx_queue, &item, 0);
            break;
        }
        // Route to RX
        case PKT_MESSAGE_DATA:
        {
            uint8_t payload_len = buf[LEN_POS];

            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_MESSAGE_DATA", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            printf("[0x%02X] %-20s : LEN:0x%02X DATA:", seq, "INFO_RX_MESSAGE_DATA", payload_len);
            for (uint8_t i = 0U; i < payload_len; i++) {
                printf("0x%02X ", buf[PAYLOAD_POS + i]);
            }
            printf("\n");

            // Send info to the queue
            RXQueue_Item_t item = {
                .seq         = seq,
                .type        = type,
                .payloadType = RX_PAYLOAD_MESSAGE_DATA,
            };
            memcpy(&item.payload.messageData, &buf[PAYLOAD_POS], payload_len);
            xQueueSend(rx_queue, &item, 0);
            break;
        }
        // Route to TX
        case PKT_COMMAND_ACK:
        {
            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_COMMAND_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            xTaskNotify(xTXTaskHandle, PKT_COMMAND_ACK, eSetValueWithOverwrite);
            break;
        }
        // Route to TX
        case PKT_FIRMWARE_ACK:
        {
            printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
                seq, "RX_FIRMWARE_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            xTaskNotify(xTXTaskHandle, PKT_FIRMWARE_ACK, eSetValueWithOverwrite);
            break;
        }
        default: 
            ESP_LOGW(TAG, "Unknown packet type: 0x%02X", type);
            break;
    }
}

static void uart_router_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t buf[ROUTER_BUFFER_SIZE] = {0U};
    uint8_t len = 0U;

    while (1) 
    {
        // Block on UART queue waiting for RX from STM32
        if (xQueueReceive(uart_2_queue, (void *)&event, portMAX_DELAY))     // Block wait for item on queue
        {
            if (event.type == UART_PATTERN_DET)                             // trigger on pattern EOF_BYTE detection 
            {
                int pos = uart_pattern_pop_pos(UART_NUM2);                  // get position of the triggering byte, EOF_BYTE
                if (pos != -1) {
                    len = pos + 1;                                          // record length all bytes including the EOF_BYTE

                    uart_read_bytes(UART_NUM2, buf, len, portMAX_DELAY);    // Read the bytes from UART
                    process_rx(buf, len);                                   // pass buffer for processing
                }
            }
        }
    }
}

// Initialize Router Task
void uart_router_task_init(void)
{
    xTaskCreate(uart_router_task, "uart_router_task", 4096, NULL, TASK_PRIO_6, NULL);
}