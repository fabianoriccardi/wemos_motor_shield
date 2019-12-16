#ifndef __BOOT_H
#define __BOOT_H

#include "stm32f030x6.h"
#include "stdint.h"

#define BOOTLOADER_START_ADDRESS    0x08000000
#define MAIN_PROGRAM_START_ADDRESS  0x08000C00

#define IAP_ID  0x00
#define FW_ID   0x01

#define JUMP_TO(ADDR)   do {\
                            __disable_irq();\
                            typedef void (*pFunction)(void);\
                            uint32_t JumpAddress = *(__IO uint32_t*) (ADDR + 4);\
                            pFunction Jump = (pFunction) JumpAddress;\
                            __set_MSP(*(__IO uint32_t*)ADDR);\
                            Jump();\
                        } while (0)

extern __IO uint32_t boot_flag;

uint8_t boot_cmd(uint8_t *data, uint8_t size);

#endif