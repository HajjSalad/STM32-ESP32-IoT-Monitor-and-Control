/**
 * @file  uart_driver.c
 * @brief UART2 peripheral driver for STM32 communication
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"

#include "uart_driver.h"

static const char *TAG = "UART2";

QueueHandle_t uart_2_queue;

esp_err_t uart2_init(void)
{
    const uart_config_t uart_config = {             
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t ret;
    
    // 1. Configure UART2 parameters
    ret = uart_param_config(UART_NUM2, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 2. Assign TX/RX pins
    ret = uart_set_pin(UART_NUM2, UART_2_TX, UART_2_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 3. Install UART driver with event queue
    ret = uart_driver_install(UART_NUM2, BUF_SIZE, BUF_SIZE, 10, &uart_2_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // 4. Enable pattern detection interrupt — fires UART_PATTERN_DET event on EOF_BYTE (0x55) 
    //    Signals end of packet to the UART RX task
    uart_enable_pattern_det_baud_intr(UART_NUM2, EOF_BYTE, 1, 9, 0, 0);

    // 5. Allocate pattern queue - stores positions of upto 20 detected EOF bytes - oldest is discarded
    uart_pattern_queue_reset(UART_NUM2, 20);

    // 6. Flush any garbage bytes arrived during initialization
    uart_flush(UART_NUM2);

    ESP_LOGI(TAG, "UART2 initialized\n");
    
    return ESP_OK;
}




// /**
//  * @file uart2_driver.c
//  * @brief
//  * 
// */

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/uart.h"
// #include "esp_system.h"
// #include "string.h"
// #include "stdio.h"

// #include "uart.h"

// QueueHandle_t uart_2_queue;

// esp_err_t uart2_init(void)
// {
//     const uart_config_t uart_config = {             
//         .baud_rate = 115200,
//         .data_bits = UART_DATA_8_BITS,
//         .parity = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
//     };

//     esp_err_t ret;
    
//     ret = uart_param_config(UART_NUM2, &uart_config);
//     if (ret != ESP_OK) return ret;

//     ret = uart_set_pin(UART_NUM2, UART_2_TX, UART_2_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
//     if (ret != ESP_OK) return ret;

//     ret = uart_driver_install(UART_NUM2, BUF_SIZE, BUF_SIZE, 10, &uart_2_queue, 0);
//     return ret;
// }

