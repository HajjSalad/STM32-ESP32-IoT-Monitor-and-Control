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
    PWR->CR      |= PWR_CR_DBP;             // 2. Enable access to backup domain (RTC register)
    RCC->CSR     |= RCC_CSR_LSION;          // 3. Enable LSI clock for RTC and wait for ready flag
    while(!(RCC->CSR & RCC_CSR_LSIRDY));

    // 4. Select LSI as clock source and enable RTC
    RCC->BDCR    |= RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_1;   
    
    // 5. Enter the "key" to unlock write protection
    RTC->WPR     |= 0xCA;       // first key                               
    RTC->WPR     |= 0x53;       // second key    
    
    // 6. Enter Initialization mode and wait for ready flag
    RTC->ISR     |= RTC_ISR_INIT;                          
    while(!(RTC->ISR & RTC_ISR_INITF));

    // 7. Adjust prescaler values for RTC to obtain 1 Hz
    RTC->PRER    |= 0xF9;            // sync prescaler         
    RTC->PRER    |= (0x7F<<16);      // async prescaler
    
    // RTC->TR      |= 0x130000;               
    // RTC->TR      |= 0x5700;             
    // RTC->DR      |= 0x215124;

    // 8. Write time and date values
    RTC->TR = 0x133000;                       // 13:30:00
    RTC->DR = 0x00260325;                     // 26/03/25 Wednesday       

    // 9. Bypass shadow registers for direct register read
    RTC->CR      |= RTC_CR_BYPSHAD;         

    // 10. Exit initialization mode
    RTC->ISR     &= ~RTC_ISR_INIT;         

    // 11. Disable access to RTC registers - Re-enable write protection
    PWR->CR &= ~PWR_CR_DBP;
}

void RTC_get_time(void) 
{
    sTime.Seconds = (((RTC->TR & 0x7f) >> 4)*10) + (RTC->TR & 0xf);
    sTime.Minutes = ((RTC->TR & 0x7f00) >> 8);
    sTime.Minutes = (((sTime.Minutes & 0x7f) >> 4)*10) + (sTime.Minutes & 0xf);
    sTime.Hours = (RTC->TR & 0x7f0000) >> 16;
    sTime.Hours = (((sTime.Hours & 0x7f) >> 4)*10) + (sTime.Hours & 0xf);
}

void RTC_get_date(void) 
{
    sDate.Year  = ((RTC->DR >> 20)*10) + ((RTC->DR >> 16) & 0xf);
    sDate.Month = ((RTC->DR >> 12) & 1)*10 + ((RTC->DR >> 8) & 0xf);
    sDate.Day   = (((RTC->DR >> 4) & 3)*10) + (RTC->DR & 0xf);
}