#include "stm32f030x6.h"
#include "common.h"
#include "user_i2c.h"
#include "boot.h"


typedef struct {
    uint8_t addr;
    uint8_t mask;
    uint8_t cmd;
    uint8_t (* handler)(uint8_t *data, uint8_t size);
} handler_t;

static uint8_t do_update(uint8_t *data, uint8_t size);
static uint8_t get_id(uint8_t *data, uint8_t size);


handler_t commands[] = {
    {0x20, 0xFF, CMD_WRITE, boot_cmd},
    {0x30, 0xFF, CMD_WRITE, do_update},
    {0x40, 0xFF, CMD_READ,  get_id},
};


static uint8_t do_update(uint8_t *data, uint8_t size)
{
    if (size < 4) return 0;

    return 0;
}

static uint8_t get_id(uint8_t *data, uint8_t size)
{
    if (size < 1) return 0;
   *(uint8_t *)data = IAP_ID;

    return 1;
}

uint8_t handle_cmd(uint8_t cmd, uint8_t *buf, uint8_t size)
{
    uint8_t addr = buf[0];
    uint8_t len = 0;

    for (uint8_t i = 0; i < array_len(commands); i++) {
        if (addr == commands[i].addr && cmd == commands[i].cmd) {
            if (commands[i].handler) {
                len = commands[i].handler(buf, size);
            }
        }
    }

    return len;
}