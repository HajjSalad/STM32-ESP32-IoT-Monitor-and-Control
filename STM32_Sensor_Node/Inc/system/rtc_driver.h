#ifndef RTC_DRIVER_H_
#define RTC_DRIVER_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t Hours;
    uint8_t Minutes;
    uint8_t Seconds;
} RTC_Time_t;

typedef struct {
    uint8_t Month;
    uint8_t Day;
    uint8_t Year;
} RTC_Date_t;

extern RTC_Time_t sTime;
extern RTC_Date_t sDate;

// Function Prototypes
void RTC_init();
void RTC_get_time(void);
void RTC_get_date(void);

#endif  // RTC_DRIVER_H_