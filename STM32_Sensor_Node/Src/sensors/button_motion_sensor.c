/**
 * @file  button_motion_sensor.c
 * @brief Motion sensor driver
 * 
 * Digital GPIO input = 0 or 1
 * EXTI input interrupt
 *
*/
#include "stm32f446xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "sensor_drivers.h"
#include "shared_resources.h"

volatile uint8_t motion_detected;  /* Motion detected : 0/1 — GPIO */

/**
 * @brief EXTI15_10 interrupt handler — motion sensor detection
*/
void EXTI15_10_IRQHandler(void)
{

    if (EXTI->PR & (1U<<12))        // Check if EXTI12 triggered
    {
        EXTI->PR |= (1U<<12);       // Clear pending flag by writing 1

        // Read current pin state to determine press or release
        if (!(GPIOB->IDR & (1U<<12))) {
            motion_detected = 1U;                    // falling edge -> motion detected
        } else {
            motion_detected = 0U;                    // rising edge -> motion cleared
        }
    }
}