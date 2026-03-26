/**
 * @file  uart_rx_task.c
 * @brief Receives items from RX queue and sends ACK packets back to the sender.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_system.h"
#include <stdint.h>
#include "string.h"
#include "stdio.h"

#include "crc_16.h"
#include "uart_driver.h"
#include "uart_tasks.h"
#include "shared_resources.h"

static const char *TAG = "RX";

// Static Function Prototypes
static void send_ack(uint8_t seq, uint8_t type);

static void uart_rx_task(void *pvParameters)
{
    RXQueue_Item_t rx_data = {0};

    while(1) 
    {
        // Block until router places an item on the queue
        if (xQueueReceive(rx_queue, &rx_data, portMAX_DELAY)) 
        {
            // Send appropriate ACK based on packet type
            switch(rx_data.type)
            {
                case PKT_HANDSHAKE_REQ: send_ack(rx_data.seq, PKT_HANDSHAKE_ACK);    break;
                case PKT_TEMP_DATA:     send_ack(rx_data.seq, PKT_TEMP_DATA_ACK);    break;
                case PKT_MOTION_DATA:   send_ack(rx_data.seq, PKT_MOTION_DATA_ACK);  break;
                case PKT_MESSAGE_DATA:  send_ack(rx_data.seq, PKT_MESSAGE_DATA_ACK); break;
                default:
                    printf("[0x%02X] RX UNKNOWN type:received\n", rx_data.seq);
                    break;
            }
        }
    }
}

// Build and transmit an ACK packet
static void send_ack(uint8_t seq, uint8_t type)
{
    UART_ACK_Packet_t pkt;

    pkt.sof       = SOF_BYTE;
    pkt.version   = PROTOCOL_VERSION;
    pkt.device_id = DEVICE_ID_ESP32;
    pkt.seq_num   = seq;
    pkt.type      = type;

    // Compute CRC over all fields except crc and eof (3 bytes)
    uint16_t crc  = compute_crc((uint8_t*)&pkt, sizeof(pkt) - 3);
    pkt.crc_high  = (uint8_t)(crc >> 8U);       // high byte first
    pkt.crc_low   = (uint8_t)(crc & 0xFFU);     // low byte second
    pkt.eof       = EOF_BYTE;

    // Transmit the packet
    uart_write_bytes(UART_NUM2, (uint8_t*)&pkt, sizeof(pkt));

    // Print to terminal the sent ACK packet
    const char *packet;
    switch (type) {
        case PKT_HANDSHAKE_ACK:    packet = "TX_HANDSHAKE_ACK";    break;
        case PKT_TEMP_DATA_ACK:    packet = "TX_TEMP_DATA_ACK";    break;
        case PKT_MOTION_DATA_ACK:  packet = "TX_MOTION_DATA_ACK";  break;
        case PKT_MESSAGE_DATA_ACK: packet = "TX_MESSAGE_DATA_ACK"; break;
        default:                   packet = "UNKNOWN";      break;
    }

    printf("[0x%02X] %-20s : SOF:0x%02X VER:0x%02X DEV:0x%02X SEQ:0x%02X TYPE:0x%02X CRC:[0x%02X 0x%02X] EOF:0x%02X\n", 
        seq, packet, pkt.sof, pkt.version, pkt.device_id, pkt.seq_num, pkt.type, pkt.crc_high, pkt.crc_low, pkt.eof);
}

void uart_rx_task_init(void)
{
    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, TASK_PRIO_5, NULL);
}