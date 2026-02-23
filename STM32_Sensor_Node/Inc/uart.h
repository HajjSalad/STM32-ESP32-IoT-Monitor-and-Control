#ifndef UART_H_
#define UART_H_

/**
 * @file uart.h
 * @brief Public API for UART2 peripheral.
*/

#include <stdio.h>
#include <stdint.h>

/** @brief Format for printf */
#define LOG(fmt, ...)  printf( (fmt "\n\r"), ##__VA_ARGS__)

// Function Prototypes
void uart2_init(void);
void uart2_write(int ch);
void uart1_init(void);
void uart1_write(int ch);
void uart1_write_string(const char *str);
char uart1_read(void);

#endif /* UART_H_ */
