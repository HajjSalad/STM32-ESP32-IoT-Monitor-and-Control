/**
 * @file  exti_driver.c
 * @brief External Interrupt (EXTI) configuration for motion sensor input.
*/

#include "exti_driver.h"
#include "stm32f446xx.h"

/**
 * @brief Initialize external interrupt for motion sensor input.
 * 
 * Configures GPIO pin PB12 as input with internal pull-up resistor
 * and maps it to EXTI line 12.
 * Both falling and rising edge triggers enabled to detect press events.
 * 
 * EXTI line unmasked and routed through NVIC via EXTI15_10 channel.
*/
void exti_init(void)
{ 
    // 1. Disable global interrupts
    __disable_irq();

    // 2. Enable GPIOB and SYSCFG clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 3. Configure PB12 as input mode (00)
    GPIOB->MODER &= ~(1U<<24);
	GPIOB->MODER &= ~(1U<<25);

    // 4. Enable pull-up resistor on PB12 (01)
    GPIOB->PUPDR &= ~(1U<<25);
	GPIOB->PUPDR |=  (1U<<24);

    // 5. Map EXTI12 to PORTB - EXTICR[3] bits [3:0] = 0001
    SYSCFG->EXTICR[3] |= (1U<<0);

    // 6. Unmask EXTI12
    EXTI->IMR |= (1u<<12);

    // 7. Select edge trigger
    EXTI->FTSR |= (1U<<12);                 // Trigger on falling edge
    EXTI->RTSR |= (1U<<12);                 // Release on rising edge

    // 8. Enable EXTI 10-15 lines in NVIC
    NVIC_SetPriority(EXTI15_10_IRQn, 6);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    // 9. Enable global interrupts
	__enable_irq();
}