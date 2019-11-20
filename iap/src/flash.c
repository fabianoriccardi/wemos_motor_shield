#include "stm32f030x6.h"
#include "flash.h"
#include <stdint.h>


void flash_write(uint32_t page_addr, uint8_t *data, uint32_t size)
{
    if (size > 1024) size = 1024;

    /* Unlock */
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;

    while (FLASH->SR & FLASH_SR_BSY);
    if (FLASH->SR & FLASH_SR_EOP) {
        FLASH->SR = FLASH_SR_EOP;
    }

    /* Page erase */
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = page_addr;
    FLASH->CR |= FLASH_CR_STRT;

    while (!(FLASH->SR & FLASH_SR_EOP));
    FLASH->SR = FLASH_SR_EOP;

    FLASH->CR &= ~FLASH_CR_PER;

    /* Page write */
    FLASH->CR |= FLASH_CR_PG;
    for (uint32_t i = 0; i < size; i += 2) {
        *(__IO uint16_t*)(page_addr + i) = *(uint16_t*)(data + i);
        while (!(FLASH->SR & FLASH_SR_EOP));
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH->CR &= ~FLASH_CR_PG;
    /* Lock */
    FLASH->CR |= FLASH_CR_LOCK;
}