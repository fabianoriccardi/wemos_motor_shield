#include "boot.h"

__IO uint32_t boot_flag __attribute__ ((section (".ram_flags")));


uint8_t boot_cmd(uint8_t *data, uint8_t size)
{
    if (size < 2) return 0;

    if (data[1] == IAP_ID) {
        boot_flag = 0xAABBCCDD;
    } else {
        boot_flag = 0;
    }

    JUMP_TO(BOOTLOADER_START_ADDRESS);

    return 0;
}