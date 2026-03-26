/**
 * @file  task_uart_rx.c
 * @brief  
*/

#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdint.h>

#include "crc_16.h"
#include "uart_driver.h"
#include "tasks.h"
#include "shared_resources.h"

// Static function prototypes
static void send_ack(uint8_t seq, uint8_t type);

void vTaskRX(void *pvParameters)
{
    (void)pvParameters;                 // Suppress unused parameter warning

    uint32_t   tick_count    = 0U; 
    BaseType_t    xRet          = pdFALSE;
    RXQueue_Item_t queueItem    = {0U};

    //printf("[RX] Stack HWM: %lu words remaining\n", uxTaskGetStackHighWaterMark(NULL));

    while(1)
    {
        // Block here until data is available to receive
        xRet = xQueueReceive(xRXQueue, &queueItem, portMAX_DELAY);
        if (xRet != pdTRUE) {
            LOG("RX queue receive failed unexpectedly");       // never reached
            continue;
        }
        
        switch(queueItem.type)
        {
            case PKT_HANDSHAKE_REQ: send_ack(queueItem.seq, PKT_HANDSHAKE_ACK);    break;
            case PKT_COMMAND:       send_ack(queueItem.seq, PKT_COMMAND_ACK);    break;
            case PKT_FIRMWARE:      send_ack(queueItem.seq, PKT_FIRMWARE_ACK); break;
            default:
                LOG("[0x%02X] RX UNKNOWN type:received", queueItem.seq);
                break;
        }

        if (tick_count++ % 100 == 0) {
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            LOG("UART_RX stack watermark: %u words remaining", watermark);
        }
    }
}

// Build and transmit an ACK packet
static void send_ack(uint8_t seq, uint8_t type)
{
    UART_ACK_Packet_t pkt;

    pkt.sof     = SOF_BYTE;
    pkt.version = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_STM32;
    pkt.seq_num = seq;
    pkt.type    = type;

    // Compute CRC over all fields except crc and eof (3 bytes)
    uint16_t crc  = compute_crc((uint8_t*)&pkt, sizeof(pkt) - 3);
    pkt.crc_high  = (uint8_t)(crc >> 8U);       // high byte first
    pkt.crc_low   = (uint8_t)(crc & 0xFFU);     // low byte second
    pkt.eof     = EOF_BYTE;

    uart1_write(pkt.sof);
    uart1_write(pkt.version);
    uart1_write(pkt.device_id);
    uart1_write(pkt.seq_num);
    uart1_write(pkt.type);
    uart1_write(pkt.crc_high);
    uart1_write(pkt.crc_low);
    uart1_write(pkt.eof);

    // Print to terminal the sent ACK packet
    const char *packet;
    switch (type) {
        case PKT_HANDSHAKE_ACK:  packet = "TX_HANDSHAKE_ACK";  break;
        case PKT_COMMAND_ACK:    packet = "TX_COMMAND_ACK";    break;
        case PKT_FIRMWARE_ACK:   packet = "TX_FIRMWARE_ACK";   break;
        default:                 packet = "UNKNOWN";           break;
    }

    LOG("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X", 
        seq, packet, pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
}



