#include "stm32f030x6.h"
#include "common.h"
#include "user_i2c.h"
#include "boot.h"
#include "flash.h"
#include <stdint.h>


uint8_t data_buf[FLASH_PAGE_SIZE];

static uint32_t fw_len = FLASH_PAGES_COUNT * FLASH_PAGE_SIZE;
static uint32_t fw_offset = 0;

typedef struct {
    uint8_t addr;
    uint8_t mask;
    uint8_t cmd;
    uint8_t (* handler)(uint8_t *data, uint8_t size);
} handler_t;

static uint8_t write_flash(uint8_t *data, uint8_t size);
static uint8_t read_flash(uint8_t *data, uint8_t size);
static uint8_t set_fwlen(uint8_t *data, uint8_t size);
static uint8_t get_fwlen(uint8_t *data, uint8_t size);
static uint8_t get_id(uint8_t *data, uint8_t size);


handler_t commands[] = {
    {0x20, 0xFF, CMD_WRITE, boot_cmd},
    {0x30, 0xF0, CMD_WRITE, write_flash},
    {0x30, 0xFE, CMD_READ,  read_flash},
    {0x40, 0xFF, CMD_READ,  get_id},
    {0x50, 0xFF, CMD_WRITE, set_fwlen},
    {0x50, 0xFF, CMD_READ,  get_fwlen},
};


static inline uint16_t reverse16(uint16_t value)
{
    return (((value & 0x00FF) << 8) |
            ((value & 0xFF00) >> 8));
}

static inline uint32_t reverse32(uint32_t value) 
{
    return (((value & 0x000000FF) << 24) |
            ((value & 0x0000FF00) <<  8) |
            ((value & 0x00FF0000) >>  8) |
            ((value & 0xFF000000) >> 24));
}

static uint8_t write_flash(uint8_t *data, uint8_t size)
{
    static uint8_t page = 0;
    static uint16_t index = 0;

    uint16_t pkt_num = ((data[0] & 0x0F) << 8) | data[1];
    data += 2;
    size -= 2;

    /* Copy first part of data packet */
    while (index < FLASH_PAGE_SIZE && size) {
        data_buf[index++] = *data++;
        size--;
    }

    /* Write sector if buffer is full */
    if (index == FLASH_PAGE_SIZE || pkt_num == 0) {
        uint32_t page_addr = MAIN_PROGRAM_START_ADDRESS + page * FLASH_PAGE_SIZE;
        if (page_addr < MAIN_PROGRAM_START_ADDRESS + (FLASH_PAGES_COUNT - 1) * FLASH_PAGE_SIZE) {
            page++;
        } else {
            page = 0;
        }
        flash_write(page_addr, data_buf, index);
        index = 0;
    }

    /* Copy the rest of data packet */
    while (size--) {
        data_buf[index++] = *data++;
    }

    return 0;
}

static uint8_t set_fwlen(uint8_t *data, uint8_t size)
{
    if (size < 5) return 0;

    uint32_t len = (uint32_t)data[1] << 24 |
        (uint32_t)data[2] << 16 |
        (uint32_t)data[3] << 8 |
        (uint32_t)data[4];

    if (size <= FLASH_PAGES_COUNT * FLASH_PAGE_SIZE) {
        fw_len = len;
        fw_offset = 0;
    }

    return 0;
}

static uint8_t get_fwlen(uint8_t *data, uint8_t size)
{
    if (size < 4) return 0;
    *(uint32_t *)data = reverse32(fw_len);
    fw_offset = 0;

    return 4;
}

static uint8_t read_flash(uint8_t *data, uint8_t size)
{
    uint16_t len = 0;
    for (len = 0; len < 30; ) {
        uint8_t *ptr = (uint8_t *)(MAIN_PROGRAM_START_ADDRESS + fw_offset++);
        len++;
        if (fw_offset >= fw_len) {
            fw_offset = 0;
            return len;
        }
        *data++ = *ptr;
    }

    return len;
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
        if ((addr & commands[i].mask) == commands[i].addr && cmd == commands[i].cmd) {
            if (commands[i].handler) {
                len = commands[i].handler(buf, size);
                break;
            }
        }
    }

    return len;
}