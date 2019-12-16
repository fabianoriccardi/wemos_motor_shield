PORT ?= /dev/ttyUSB0

CC = arm-none-eabi-gcc
GDB = arm-none-eabi-gdb
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

CFLAGS = -Wall -g -std=c99 -Os
CFLAGS += -mlittle-endian -mcpu=cortex-m0 -march=armv6-m -mthumb
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections -Wl,-Map=$(FW_NAME).map
CFLAGS += -I../inc


all: $(FW_NAME).elf

$(FW_NAME).elf: $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@ -Tstm32f030.ld
	$(OBJCOPY) -O binary $(FW_NAME).elf $(FW_NAME).bin
	$(SIZE) $(FW_NAME).elf

program: $(FW_NAME).bin
	openocd -f ../stm32f0motor.cfg -f stm32f0-openocd.cfg -c "program $(FW_NAME).bin verify reset exit $(LOAD_ADDR)" -c shutdown

gdb: $(FW_NAME).elf
	$(GDB) -x .gdbinit

flash: $(FW_NAME).bin
	stm32flash $(PORT) -k || true
	stm32flash $(PORT) -u || true
	stm32flash $(PORT) -v -w $(FW_NAME).bin

clean:
	rm -f *.o
	rm -f *.elf
	rm -f *.bin
	rm -f *.map