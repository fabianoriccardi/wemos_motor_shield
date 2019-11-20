#ifndef __FLASH_H
#define __FLASH_H

#include <stdint.h>


void flash_write(uint32_t page_addr, uint8_t *data, uint32_t size);

#endif