//
//    control_task.c - Control all devices and sensors based on input.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/uart_struct.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "demo.h"
#include "wrapper.h"
#include "priorities.h"
#include "control_task.h"
#include "main.h"

extern SemaphoreHandle_t UARTSemaphore;

void* rooms[ROOM_NUM];          // Room pointers
// Create 2 LivingRooms and 2 BedRooms
void createRooms(void) {
    for (int i=0; i < ROOM_NUM; i++) {
        int roomNum = 101 + i;
        if (i % 2 == 0) {
            rooms[i] = createLivingRoom(roomNum);
            if(!rooms[i]) {
                printf("LivingRoom creation failed.\n");
                return;
            } else {
                printf("LivingRoom %d created.\n\r", roomNum);
            }
        } else {
            rooms[i] = createBedRoom(roomNum);
            if(!rooms[i]) {
                printf("BedRoom creation failed.\n");
                return;
            } else {
                printf("BedRoom %d created.\n\r", roomNum);
            }
        }
        turnOffLight(rooms[i]);
        turnOffAC(rooms[i]);
        turnOffHeater(rooms[i]);
    }
}

uint32_t bufferSend[SENSOR_NUM];              // Buffer to store values to Send to AWS
void setSensorValues(void) {
    for (int i=0; i < SENSOR_NUM; i+=2) {
        int roomIndex = i / 2; 
        void* room = rooms[roomIndex];
        int tempValue = (rand() % 11) + 15;         // Generate a number 15-25
        setTempSensorValue(room, tempValue);        
        bufferSend[i] = tempValue;                  // Store the temp value
        printf("Room %u temp value: %u\n\r", 101+roomIndex, tempValue);
        int motionValue = (rand() % 2);             // Generate a number 0-1
        setMotionDetectorValue(room, motionValue);
        bufferSend[i+1] = motionValue;                  // Store the temp value
        printf("Room %u motion value: %u\n\r", 101+roomIndex, motionValue);
    }
    for (int i=0; i < SENSOR_NUM; i++) {
        printf("bufferSend[%d] = %ld\n\r", i, bufferSend[i]);
    }
}

void updateDevices(void) {
    for (int i = 0; i < ROOM_NUM; i++) {
        void* room = rooms[i];  
        if (getMotionDetectorValue(room)) {         // Motion Detector Logic
            if (!isLightOn(room)) {
                turnOnLight(room);
                printf("Light turned On Room %d\n\r", 101 + i);
            }
        } else {
            if (isLightOn(room)) {
                turnOffLight(room);
                printf("Light turned Off Room %d\n\r", 101 + i);
            } else {
                printf("Light is Off Room %d\n\r", 101 + i);
            }
        }

        int temp = getTempSensorValue(room);  
        if (temp > 25) {             // Temperature Sensor Logic
            turnOnAC(room);
            turnOffHeater(room);
            printf("AC turned On Room %d\n\r", 101 + i);
        } else if (temp < 20) {
            turnOffAC(room);
            turnOnHeater(room);
            printf("Heater turned On Room %d\n\r", 101 + i);
        } else {  // temp is between 20 and 25
            turnOffAC(room);
            turnOffHeater(room);
            printf("Heater and AC turned Off Room %d\n\r", 101 + i);
        }
    }
}

static void controlTask(void *arg){

    createRooms();              // Create 4 Rooms
    srand(time(NULL));          // Seed random number generator
    setSensorValues();          // Set sensor values

    while(1) {

        setSensorValues();                  // Set sensor values
        updateDevices();                    // Update devices on sensor values 
        post_rest_function(bufferSend);     // Send data to AWS IoT

        if (xSemaphoreTake(UARTSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            printf("Control Task Running...\n\r");
            xSemaphoreGive(UARTSemaphore);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Initialize the Control task.
uint32_t controlTaskInit(void) {
    if (xTaskCreate(controlTask, "control_task", CONTROL_TASK_STACK_SIZE * 2, NULL, PRIORITY_CONTROL_TASK, NULL)) {
        printf("Control Task created..!\n\r");
        return 1;
    } else {
        printf("Control Task NOT created..!\n\r");
        return 0;
    }
}

