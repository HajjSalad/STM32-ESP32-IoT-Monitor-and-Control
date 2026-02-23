#ifndef WIFI_H
#define WIFI_H

/**
 * @file wifi.c
 * @brief 
*/

#define SSID "TELUS8523"
#define PASS "b3AEB7uamch4"

// Function Prototype
void wifi_init(void);
void wifi_start(void);
void wifi_stop(void);
bool wifi_is_connected(void);
void wifi_wait_until_connected(void);

void wifi_manager_task_init(void);

#endif  // WIFI_H