/**
 * @file  sensor_drivers.h
 * @brief 
*/

/* ---  TMP102 Temp Sensor --- */
typedef enum {
    TMP102_OK    = 0,
    TMP102_ERROR = 1
} TMP102_Status_t;

TMP102_Status_t tmp102_init(void);
TMP102_Status_t tmp102_read(float *temp);

/* ---  Motion sensor - Digital GPIO input --- */
typedef enum {
    MOTION_OK    = 1,
    MOTION_ERROR = 0
} Motion_Status_t;

typedef enum {
    MOTION_NOT_DETECTED = 0,
    MOTION_DETECTED     = 1
} Motion_State_t;