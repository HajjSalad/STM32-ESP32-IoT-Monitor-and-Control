//
// control_task.h - Prototypes for the Control task.
//

#ifndef CONTROL_TASK_H
#define CONTROL_TASK_H

#include <stdint.h>

#define ROOM_NUM                  4         // Number of rooms
#define SENSOR_NUM                8         // 4 rooms * 2 sensors each = 8 sensors
#define CONTROL_TASK_STACK_SIZE   3072

extern SemaphoreHandle_t UARTSemaphore;

uint32_t controlTaskInit(void);

#endif // CONTROL_TASK_H