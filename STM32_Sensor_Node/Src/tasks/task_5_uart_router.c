/**
 * @file  task_4_uart_router.c
 * @brief 
*/

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"

#include <string.h>
#include <stdint.h>

#include "tasks.h"
#include "crc_16.h"
#include "uart_driver.h"
#include "shared_resources.h"

#define ROUTER_BUFFER_SIZE     128

volatile uint8_t task5_alive = 0U;

TaskHandle_t         xRouterTaskHandle  = NULL;
StreamBufferHandle_t xUartStreamBuffer  = NULL;

volatile uint8_t  rxBuffer[UART_RX_BUFFER_SIZE];
volatile uint16_t rxHead = 0;
volatile uint16_t rxTail = 0;

static void process_rx(const uint8_t *buf, uint8_t len) 
{
    if (buf[SOF_POS]  != SOF_BYTE) return;         // Validate SOF
    if (buf[len - 1U] != EOF_BYTE) return;         // Validate EOF

    uint8_t type     = buf[TYPE_POS];
    uint8_t crc_high = buf[len - 3U];
    uint8_t crc_low  = buf[len - 2U];

    // Validate CRC
    if (type == PKT_COMMAND) {
        uint8_t payload_len = buf[LEN_POS];
        uint8_t crc_len = 6 + payload_len;

        uint16_t received_crc = ((uint16_t)(crc_high << 8U) | ((uint16_t)crc_low));
        uint16_t computed_crc = compute_crc(buf, crc_len);

        if (received_crc != computed_crc) {
        LOG("CRC FAIL - packet discarded");
        LOG("Received CRC: 0x%02X 0x%02X", crc_high, crc_low);
        LOG("Computed CRC: 0x%02X 0x%02X", (computed_crc >> 8), (computed_crc & 0x00FF));
        return;
    }
    }

    // printf("Buffer [%u bytes]: ", len);
    // for (uint8_t i = 0U; i < len; i++) {
    //     printf("0x%02X ", buf[i]);
    // }
    // printf("\n\r");
    //
    // Validate CRC
    // uint16_t received_crc = ((uint16_t)(crc_high << 8U) | ((uint16_t)crc_low));
    // uint16_t computed_crc = compute_crc(buf, len - 3U);
    // if (received_crc != computed_crc) {
    //     LOG("CRC FAIL - packet discarded");
    //     LOG("Received CRC: 0x%02X 0x%02X", crc_high, crc_low);
    //     LOG("Computed CRC: 0x%02X 0x%02X", (computed_crc >> 8), (computed_crc & 0x00FF));
    //     return;
    // }

    uint8_t sof         = buf[SOF_POS];
    uint8_t ver         = buf[VER_POS];
    uint8_t dev         = buf[DEV_POS];
    uint8_t seq         = buf[SEQ_POS];
    uint8_t eof         = buf[len - 1U];

    switch(type) 
    {
        // Route to RX
        case PKT_HANDSHAKE_REQ:                     
        {
            LOG("\n--- Starting new RX cycle ---");
            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_HANDSHAKE_REQ", sof, ver, dev, seq, type, crc_high, crc_low, eof);

            RXQueue_Item_t queueItem = {
                .seq  = seq, 
                .type = type,
            };
            xQueueSend(xRXQueue, &queueItem, 0);
            break;
        }
        // Route to RX
        case PKT_COMMAND:
        {
            uint8_t payload_len = buf[LEN_POS];

            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_COMMAND_DATA", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            printf("[0x%02X] %-20s : LEN:0x%02X DATA:", seq, "INFO_RX_COMMAND", payload_len);
            for (uint8_t i = 0U; i < payload_len; i++) {
                printf("0x%02X ", buf[PAYLOAD_POS + i]);
            }
            printf("\n\r");

            // Send info to queue
            RXQueue_Item_t queueItem = {
                .seq = seq, 
                .type = type, 
                .payloadType = RX_PAYLOAD_COMMAND,
            };
            memcpy(&queueItem.payload.command, &buf[PAYLOAD_POS], payload_len);
            xQueueSend(xRXQueue, &queueItem, 0);
            break;
        }
        // Route to RX
        case PKT_FIRMWARE:
        {
            uint8_t payload_len = buf[LEN_POS];

            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_FIRMWARE_DATA", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            printf("[0x%02X] %-20s : LEN:0x%02X DATA:", seq, "INFO_RX_FIRMWARE", payload_len);
            for (uint8_t i = 0U; i < 15; i++) {     // Just print the first 15 bytes instead of the 128 bytes firmware chunk
                printf("0x%02X ", buf[PAYLOAD_POS + i]);
            }
            printf("\n\r");

            RXQueue_Item_t queueItem = {
                .seq = seq, 
                .type = type, 
                .payloadType = RX_PAYLOAD_FIRMWARE,
            };
            memcpy(&queueItem.payload.firmware, &buf[PAYLOAD_POS], payload_len);
            xQueueSend(xRXQueue, &queueItem, 0);
            break;
        }
        // Route to TX
        case PKT_HANDSHAKE_ACK:
        {
            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_HANDSHAKE_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);

            xTaskNotify(xTXTaskHandle, PKT_HANDSHAKE_ACK, eSetValueWithOverwrite);
            break;
        }
        // Route to TX
        case PKT_TEMP_DATA_ACK:
        {
            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_TEMP_DATA_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            xTaskNotify(xTXTaskHandle, PKT_TEMP_DATA_ACK, eSetValueWithOverwrite);
            break;
        }
        //Route to TX
        case PKT_MOTION_DATA_ACK:
        {
            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_MOTION_DATA_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            xTaskNotify(xTXTaskHandle, PKT_MOTION_DATA_ACK, eSetValueWithOverwrite);
            break;
        }
        // Route to TX
        case PKT_MESSAGE_DATA_ACK:
        {
            LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
                seq, "RX_MESSAGE_DATA_ACK", sof, ver, dev, seq, type, crc_high, crc_low, eof);
            
            xTaskNotify(xTXTaskHandle, PKT_MESSAGE_DATA_ACK, eSetValueWithOverwrite);
            break;
        }
        default: 
            break;
    }
}

void vTaskRouter(void *pvParameters) 
{
    uint8_t  byte;
    uint8_t  buf[ROUTER_BUFFER_SIZE] = {0U};
    uint8_t  buf_index = 0U;

    size_t   bytes_read;
    uint8_t  chunk[32];

    BaseType_t xRet = pdFALSE;
    char msg[LOG_MSG_MAX_LEN];

    uint32_t tick_count    = 0U;

    while(1) 
    {
        // Block until 8 bytes available
        bytes_read = xStreamBufferReceive(xUartStreamBuffer, chunk, sizeof(chunk), portMAX_DELAY);

        for (size_t i = 0; i < bytes_read; i++) 
        {
            byte = chunk[i];

            if (byte == SOF_BYTE) {
                buf_index = 0;                  // reset for new packet
                buf[buf_index++] = byte;        // strat recording packet
            } else if (buf_index > 0) {
                buf[buf_index++] = byte;        // continue recording packet

                if (byte == EOF_BYTE) {         // reached end
                    process_rx(buf, buf_index);    // process packet
                    buf_index = 0;
                }
            }
        }

        // Set alive flag
        task5_alive = 1;
        // snprintf(msg, sizeof(msg), "[T5] Sent alive heartbeat");
        // xRet = xQueueSend(xLogQueue, msg, 0U);
        // if (xRet != pdTRUE) {
        //     /* Log queue full — increment error counter or set error flag */
        // }

        // while (rxHead != rxTail) 
        // {
        //     byte   = rxBuffer[rxTail];
        //     rxTail = (rxTail + 1) % UART_RX_BUFFER_SIZE;

        //     if (byte == SOF_BYTE) {
        //         buf_index = 0;                                 // reset on new packet
        //         buf[buf_index++] = byte;                       // Starting recording packets
        //     } else if (buf_index > 0) {
        //         buf[buf_index++] = byte;

        //         if (byte == EOF_BYTE) {
        //             process_rx(buf, buf_index);
        //             buf_index = 0;                                 // reset for next packet
        //         }
        //     }
        // }

        // if (tick_count++ % 100 == 0) {
        //     UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        //     LOG("UART_Router stack watermark: %u words remaining", watermark);
        // }
        // vTaskDelay(10);
    } 
}