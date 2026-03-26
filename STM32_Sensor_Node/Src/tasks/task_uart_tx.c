/**
 * @file  task_uart_tx.c
 * @brief 
*/

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "crc_16.h"
#include "uart_driver.h"
#include "tasks.h"
#include "shared_resources.h"

#define RETRY_MAX            5
#define UART_TIMEOUT_MS      5000

// Static function prototypes
static void send_handshake_request(uint8_t seq, uint8_t retry_count);
static void send_data_packet(uint8_t seq, uint8_t retry_count, const TXQueue_Item_t *pQueueItem);

TaskHandle_t xTXTaskHandle = NULL;

// Instantiate a TX context
static UART_TX_Context tx_ctx = {
    .state       = STATE_SEND_HANDSHAKE,
    .seq         = SEQ_STM32_BASE,
    .retry_count = 0U,
};

void vTaskTX(void *pvParameters)
{
    (void)pvParameters;                          // Suppress unused parameter warning

    uint32_t   tick_count    = 0U; 
    BaseType_t    xRet          = pdFALSE;
    TXQueue_Item_t queueItem    = {0U};

    while(1) {

        // Block here until data is available to send
        xRet = xQueueReceive(xTXQueue, &queueItem, portMAX_DELAY);
        if (xRet != pdTRUE) {
            LOG("TX queue receive failed unexpectedly"); continue;  // never reached
        }

        // if (queueItem.payloadType == TX_PAYLOAD_TEMP_DATA) {
        //     printf("Temp from TXQueue: 0x%02X\n", queueItem.payload.tempData);
        // } 
        // else {
        //     printf("Motion from TXQueue: 0x%02X\n", queueItem.payload.motionData);
        // }


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
                            LOG("[0x%02X] %-20s: Retrying...", tx_ctx.seq, "HANDSHAKE_ACK timeout");
                        } else {
                            tx_ctx.retry_count = 0;
                            tx_ctx.state       = STATE_IDLE;
                            LOG("[0x%02X] %-20s : After %d attempts", tx_ctx.seq, "HANDSHAKE failed", RETRY_MAX);
                            tx_ctx.seq   = (tx_ctx.seq + 1) & 0x7FU;
                            txn.result   = TX_RESULT_FAILED;
                            txn.end_tick = xTaskGetTickCount();
                            LOG("[0x%02X] %-20s : seq:0x%02X | payload:0x%02X | retries:%u | duration:%lums",
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
                            LOG("[0x%02X] %-20s : Retrying...", tx_ctx.seq, "DATA_ACK timeout");
                        } else {
                            tx_ctx.retry_count = 0;
                            tx_ctx.state       = STATE_IDLE;
                            LOG("[0x%02X] %-20s : After %d attempts", tx_ctx.seq, "DATA failed", RETRY_MAX);
                            tx_ctx.seq   = (tx_ctx.seq + 1) & 0x7FU;
                            txn.result   = TX_RESULT_FAILED;
                            txn.end_tick = xTaskGetTickCount();
                            LOG("[0x%02X] %-20s : seq:0x%02X | payload:0x%02X | retries:%u | duration:%lums",
                                txn.seq, "TXN FAILED", txn.seq, txn.payloadType, RETRY_MAX, (txn.end_tick - txn.start_tick)); }
                    } 
                    else if (notifyValue == PKT_TEMP_DATA_ACK || notifyValue == PKT_MOTION_DATA_ACK || notifyValue == PKT_MESSAGE_DATA_ACK) {
                        tx_ctx.retry_count = 0;
                        tx_ctx.state       = STATE_IDLE;
                        txn.result         = TX_RESULT_SUCCESS;
                        txn.end_tick       = xTaskGetTickCount();
                        LOG("[0x%02X] %-20s : seq:0x%02X | payload:0x%02X | retries:%u | duration:%lums",
                            txn.seq, "TXN SUCCESS", txn.seq, txn.payloadType, txn.retries, (txn.end_tick - txn.start_tick));
                        LOG("[0x%02X] --- Cycle complete ---", tx_ctx.seq);
                        tx_ctx.seq   = (tx_ctx.seq + 1) & 0x7FU;
                    }
                    break;
                }
                case STATE_IDLE:
                    break;
            }
        } 
        if (tick_count++ % 100 == 0) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            LOG("UART_TX stack watermark: %u words remaining", watermark);
        }
    }
}

static void send_handshake_request(uint8_t seq, uint8_t retry_count)
{
    UART_Handshake_Packet_t pkt;

    pkt.sof       = SOF_BYTE;
    pkt.version   = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_STM32;
    pkt.seq_num   = seq;
    pkt.type      = PKT_HANDSHAKE_REQ;  

    uint16_t crc  = compute_crc((uint8_t*)&pkt, sizeof(pkt) - 3);   // Exclude 3 last bytes (crc & eof)
    pkt.crc_high  = (uint8_t)(crc >> 8U);
    pkt.crc_low   = (uint8_t)(crc & 0xFFU);
    pkt.eof       = EOF_BYTE;

    if (retry_count == 0) {
        LOG("\n--- Starting new TX cycle ---");
        LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X",
            tx_ctx.seq, "TX_HANDSHAKE_REQ", pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
    }

    uart1_write(pkt.sof);
    uart1_write(pkt.version);
    uart1_write(pkt.device_id);
    uart1_write(pkt.seq_num);
    uart1_write(pkt.type);
    uart1_write(pkt.crc_high);
    uart1_write(pkt.crc_low);
    uart1_write(pkt.eof);
}

static void send_data_packet(uint8_t seq, uint8_t retry_count, const TXQueue_Item_t *pQueueItem)
{
    UART_Data_Packet_t pkt;

    pkt.sof       = SOF_BYTE;
    pkt.version   = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_STM32;
    pkt.seq_num   = seq;

    const char *label;

    switch(pQueueItem->payloadType)
    {
        case TX_PAYLOAD_TEMP_DATA:
            pkt.type   = PKT_TEMP_DATA;
            pkt.length = sizeof(TempData_t);
            label = "TX_TEMP_DATA";
            memcpy(pkt.payload, &pQueueItem->payload.tempData, pkt.length);
            break;

        case TX_PAYLOAD_MOTION_DATA:
            pkt.type   = PKT_MOTION_DATA;
            pkt.length = sizeof(MotionData_t);
            label = "TX_MOTION_DATA";
            memcpy(pkt.payload, &pQueueItem->payload.motionData, pkt.length);
            break;

        case TX_PAYLOAD_MESSAGE_DATA:
            pkt.type   = PKT_MESSAGE_DATA;
            pkt.length = sizeof(MessageData_t);
            label = "TX_MESSAGE_DATA";
            memcpy(pkt.payload, &pQueueItem->payload.messageData, pkt.length);
            break;

        default:
            LOG("Unknown payload type: 0x%02X", pQueueItem->payloadType);
            return;
    }

    // Get length needed for the CRC  
    uint8_t crc_len = 6U + pkt.length;
    uint16_t crc    = compute_crc((uint8_t*)&pkt, crc_len);   // Exclude 3 last bytes (crc & eof)
    pkt.crc_high    = (uint8_t)(crc >> 8U);
    pkt.crc_low     = (uint8_t)(crc & 0xFFU);
    pkt.eof         = EOF_BYTE;

    if (retry_count == 0) {
        LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X",
            tx_ctx.seq, label, pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
        
        printf("[0x%02X] INFO_%-15s : LEN:0x%02X DATA:", tx_ctx.seq, label, pkt.length);
        for (uint8_t i = 0U; i < pkt.length; i++) {
            printf("0x%02X ", pkt.payload[i]);
        }
        printf("\n\r");
    }

    uart1_write(pkt.sof);
    uart1_write(pkt.version);
    uart1_write(pkt.device_id);
    uart1_write(pkt.seq_num);
    uart1_write(pkt.type);
    uart1_write(pkt.length);
    for (uint8_t i=0; i < pkt.length; i++) {
        uart1_write(pkt.payload[i]);
    }
    uart1_write(pkt.crc_high);
    uart1_write(pkt.crc_low);
    uart1_write(pkt.eof);

    /**
     * CRC = 0x1A2B = 0001 1010  0010 1011
     *
     * DR in UART only 8 bits wide
     * 
     * Sending high byte, shift by 8:
     *   0x1A2B >> 8  =  0000 0000   0001 1010
     *                  [discarded]  [sent]
     *
     * Sending low byte:
     *   0x1A2B & 0xFF  : 0001 1010 0010 1011 & 0000 0000 1111 1111 =  0000 0000   0010 1011
     *                                                                [discarded]  [sent]
    */
}