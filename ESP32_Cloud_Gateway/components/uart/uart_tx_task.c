/**
 * @file  uart_tx_task.c
 * @brief 
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "crc_16.h"
#include "uart_driver.h"
#include "uart_tasks.h"
#include "shared_resources.h"

#define RETRY_MAX            5
#define UART_TIMEOUT_MS      1000


static const char *TAG = "TX";

// Static Function Prototypes
static void send_handshake_request(uint8_t seq, uint8_t retry_count);
static void send_data_packet(uint8_t seq, uint8_t retry_count, const TXQueue_Item_t *pQueueItem);

TaskHandle_t xTXTaskHandle = NULL;

// Instantiate a TX context
static UART_TX_Context tx_ctx = {
    .state       = STATE_SEND_HANDSHAKE,
    .seq         = SEQ_ESP32_BASE,
    .retry_count = 0U,
};

static void uart_tx_task(void *pvParameters)
{
    TXQueue_Item_t queueItem = {0};

    while(1)
    {
        if (xQueueReceive(tx_queue, &queueItem, portMAX_DELAY)) 
        {
            // Transaction begins when item dequeued
            TX_Transaction_t txn = {
                .seq         = tx_ctx.seq,
                .payloadType = queueItem.payloadType,
                .retries     = 0,
                .start_tick  = xTaskGetTickCount(),
                .end_tick    = 0,
                .result      = TX_RESULT_PENDING,
            };

            // Reset for new transaction
            tx_ctx.state       = STATE_SEND_HANDSHAKE;
            tx_ctx.retry_count = 0;

            // Run state machine to completion for each item in the queue
            while (tx_ctx.state != STATE_IDLE) 
            {
                switch(tx_ctx.state) 
            {
                case STATE_SEND_HANDSHAKE:
                {
                    send_handshake_request(tx_ctx.seq, tx_ctx.retry_count);
                    tx_ctx.retry_count++;                 
                    txn.retries++;
                    tx_ctx.state = STATE_WAIT_HANDSHAKE_ACK;     
                    break;
                }
                case STATE_WAIT_HANDSHAKE_ACK:
                {
                    uint32_t notifyValue = 0U;
                    if (xTaskNotifyWait(0, ULONG_MAX, &notifyValue, pdMS_TO_TICKS(UART_TIMEOUT_MS)) == pdFALSE) {
                        // timeout
                        if (tx_ctx.retry_count < RETRY_MAX) {
                            tx_ctx.state = STATE_SEND_HANDSHAKE;
                            printf("[0x%02X] %-20s: Retrying...\n", tx_ctx.seq, "HANDSHAKE_ACK timeout");
                        } else {
                            tx_ctx.retry_count = 0;
                            tx_ctx.state       = STATE_IDLE;
                            printf("[0x%02X] %-20s : After %d attempts\n", tx_ctx.seq, "HANDSHAKE failed", RETRY_MAX);
                            tx_ctx.seq   = (tx_ctx.seq + 1) & 0x7FU;
                            txn.result   = TX_RESULT_FAILED;
                            txn.end_tick = xTaskGetTickCount();
                            printf("[0x%02X] %-20s : seq:0x%02X | payload:0x%02X | retries:%u | duration:%lums\n",
                                txn.seq, "TXN FAILED", txn.seq, txn.payloadType, txn.retries, (txn.end_tick - txn.start_tick));
                        }
                    } else if (notifyValue == PKT_HANDSHAKE_ACK) {
                        // correct ACK received
                        tx_ctx.retry_count = 0;
                        tx_ctx.state       = STATE_SEND_DATA;
                    }
                    break;
                }
                case STATE_SEND_DATA:
                {
                    send_data_packet(tx_ctx.seq, tx_ctx.retry_count, &queueItem);
                    tx_ctx.retry_count++;
                    tx_ctx.state = STATE_WAIT_DATA_ACK;
                    break;
                }
                case STATE_WAIT_DATA_ACK:
                {
                    uint32_t notifyValue = 0U;
                    if (xTaskNotifyWait(0, ULONG_MAX, &notifyValue, pdMS_TO_TICKS(UART_TIMEOUT_MS)) == pdFALSE) {
                        // timeout
                        if (tx_ctx.retry_count < RETRY_MAX) {
                            tx_ctx.state = STATE_SEND_DATA;
                            printf("[0x%02X] %-20s : Retrying...\n", tx_ctx.seq, "DATA_ACK timeout");
                        } else {
                            tx_ctx.retry_count = 0;
                            tx_ctx.state       = STATE_IDLE;
                            printf("[0x%02X] %-20s : After %d attempts\n", tx_ctx.seq, "DATA failed", RETRY_MAX);
                            tx_ctx.seq   = (tx_ctx.seq + 1) & 0x7FU;
                            txn.result   = TX_RESULT_FAILED;
                            txn.end_tick = xTaskGetTickCount();
                            printf("[0x%02X] %-20s : seq:0x%02X | payload:0x%02X | retries:%u | duration:%lums\n",
                                txn.seq, "TXN FAILED", txn.seq, txn.payloadType, RETRY_MAX, (txn.end_tick - txn.start_tick)); }
                    } 
                    else if (notifyValue == PKT_COMMAND_ACK || notifyValue == PKT_FIRMWARE_ACK) {
                        tx_ctx.retry_count = 0;
                        tx_ctx.state       = STATE_IDLE;
                        txn.result         = TX_RESULT_SUCCESS;
                        txn.end_tick       = xTaskGetTickCount();
                        printf("[0x%02X] %-20s : seq:0x%02X payload:0x%02X retries:%u duration:%lums\n",
                            txn.seq, "TXN SUCCESS", txn.seq, txn.payloadType, txn.retries, (txn.end_tick - txn.start_tick));
                        printf("[0x%02X] --- Cycle complete ---\n", tx_ctx.seq);
                        tx_ctx.seq   = 0x80U | ((tx_ctx.seq + 1) & 0x7FU);
                    }
                    break;
                }
                case STATE_IDLE:
                    break;
            }
            }
        }
    }
}

static void send_handshake_request(uint8_t seq, uint8_t retry_count)
{
    UART_Handshake_Packet_t pkt;

    pkt.sof       = SOF_BYTE;
    pkt.version   = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_ESP32;
    pkt.seq_num   = seq;
    pkt.type      = PKT_HANDSHAKE_REQ;

    uint16_t crc  = compute_crc((uint8_t*)&pkt, sizeof(pkt) - 3);   // Exclude 3 last bytes (crc & eof)
    pkt.crc_high  = (uint8_t)(crc >> 8U);
    pkt.crc_low   = (uint8_t)(crc & 0xFFU);
    pkt.eof       = EOF_BYTE;

    if (retry_count == 0) {
        printf("\n--- Starting new TX cycle ---\n");
        printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n",
            tx_ctx.seq, "TX_HANDSHAKE_REQ", pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
    }

    uart_write_bytes(UART_NUM2, (uint8_t*)&pkt, sizeof(pkt));
}

static void send_data_packet(uint8_t seq, uint8_t retry_count, const TXQueue_Item_t *pQueueItem)
{
    UART_Data_Packet_t pkt;

    pkt.sof       = SOF_BYTE;
    pkt.version   = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_ESP32;
    pkt.seq_num   = seq;

    const char *label;

    switch(pQueueItem->payloadType)
    {
        case TX_PAYLOAD_COMMAND:
            pkt.type   = PKT_COMMAND;
            pkt.length = sizeof(CommandData_t);
            label = "TX_COMMAND_DATA";
            memcpy(pkt.payload, &pQueueItem->payload.command, pkt.length);
            break;
        case TX_PAYLOAD_FIRMWARE:
            pkt.type   = PKT_FIRMWARE;
            pkt.length = sizeof(FirmwareData_t);
            label = "TX_FIRMWARE_DATA";
            memcpy(pkt.payload, &pQueueItem->payload.firmware, pkt.length);
            break;
        default:
            printf("Unknown payload: 0x%02X\n", pQueueItem->payloadType);
            return;
    }

    // Get length needed for the CRC  
    uint8_t crc_len = 6U + pkt.length;
    uint16_t crc    = compute_crc((uint8_t*)&pkt, crc_len);   // Exclude 3 last bytes (crc & eof)
    pkt.crc_high    = (uint8_t)(crc >> 8U);
    pkt.crc_low     = (uint8_t)(crc & 0xFFU);
    pkt.eof         = EOF_BYTE;

    if (retry_count == 0) {
        printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n",
            tx_ctx.seq, label, pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
        
        printf("[0x%02X] INFO_%-15s: LEN:0x%02X DATA:", tx_ctx.seq, label, pkt.length);
        if (pkt.type == PKT_FIRMWARE) {
            for (uint8_t i = 0U; i < 15; i++) {     // Just print the first 15 bytes instead of the 128 bytes firmware chunk
                printf("0x%02X ", pkt.payload[i]);
            }
        } else {
            for (uint8_t i = 0U; i < pkt.length; i++) {
                printf("0x%02X ", pkt.payload[i]);
            }
        }
        printf("\n\r");
    }

    uart_write_bytes(UART_NUM2, (uint8_t*)&pkt, sizeof(pkt));
}

void uart_tx_task_init(void)
{
    xTaskCreate(uart_tx_task, "uart_tx_task", 4096, NULL, TASK_PRIO_5, &xTXTaskHandle);
}