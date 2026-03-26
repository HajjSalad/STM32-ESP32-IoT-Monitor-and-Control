/**
 * @file  rtc_driver.c
 * @brief 
*/

#include "stm32f446xx.h"
#include "rtc_driver.h"

#define SYS_FREQ        	((uint32_t) 16000000)     // HSI - internal oscillator, no PLL
#define APB1_CLK        	(SYS_FREQ / 2)            // APB1 = 8 MHz (APB1 prescaler = 2)

RTC_Time_t sTime;
RTC_Date_t sDate;

void RTC_init() 
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;      // 1. Enable peripheral clock power
    PWR->CR      |= PWR_CR_DBP;             // 2. Enable access to RTC register
    RCC->CSR     |= RCC_CSR_LSION;          // 3. Enable LSI clock for RTC and wait for ready flag
    while(!(RCC->CSR & RCC_CSR_LSIRDY)){}

    RCC->BDCR    |= 0x8200;                 // 4. Select LSI as clock source and enable RTC
    RTC->WPR     |= 0xCA;                   // 5. Enter the "key" to unlock write protection
    RTC->WPR     |= 0x53;                   
    RTC->ISR     |= RTC_ISR_INIT;           // 6. Set INIT bit and wait for ready flag
    while(!(RTC->ISR & RTC_ISR_INITF)){}

    RTC->PRER    |= 0xF9;                   // 7. Adjust prescaler values for RTC to obtain 1 Hz
    RTC->PRER    |= 0x7F<<16;
    
    // RTC->TR      |= 0x130000;               // 8. Write time and date values
    // RTC->TR      |= 0x5700;             
    // RTC->DR      |= 0x215124;
    RTC->TR = 0x133000;                     // 13:30:00
    RTC->DR = 0x00263325;                     // 26/03/25 Wednesday       

    RTC->CR      |= RTC_CR_BYPSHAD;         // 9. Set BYPSHAD bit
    RTC->ISR     &= ~RTC_ISR_INIT;          // 10. Clear INIT bit
    PWR->CR      &= ~PWR_CR_DBP;            // 11. Disable access to RTC registers
}

void RTC_get_time(void) {
    sTime.Seconds = (((RTC->TR & 0x7f) >> 4)*10) + (RTC->TR & 0xf);
    sTime.Minutes = ((RTC->TR & 0x7f00) >> 8);
    sTime.Minutes = (((sTime.Minutes & 0x7f) >> 4)*10) + (sTime.Minutes & 0xf);
    sTime.Hours = (RTC->TR & 0x7f0000) >> 16;
    sTime.Hours = (((sTime.Hours & 0x7f) >> 4)*10) + (sTime.Hours & 0xf);
}

void RTC_get_date(void) {
    sDate.Year  = ((RTC->DR >> 20)*10) + ((RTC->DR >> 16) & 0xf);
    sDate.Month = ((RTC->DR >> 12) & 1)*10 + ((RTC->DR >> 8) & 0xf);
    sDate.Day   = (((RTC->DR >> 4) & 3)*10) + (RTC->DR & 0xf);
}