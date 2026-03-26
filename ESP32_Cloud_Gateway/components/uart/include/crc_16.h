#ifndef CRC_16_H
#define CRC_16_H

/**
 * @file  crc_16.h
 * @brief 
 * 
*/

#include <stdint.h>
#include <stddef.h>

// Function prototypes
uint16_t compute_crc(const uint8_t * pData, size_t numBytes); 

#endif      // CRC_16_H