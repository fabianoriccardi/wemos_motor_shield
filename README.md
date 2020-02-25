# Wemos Motor Shield Firmware
Alternative firmware for a popular motor shield from Wemos Electronics.

Both HW revisions ([v1.0.0](https://wiki.wemos.cc/products:retired:motor_shield_v1.0.0) and [v2.0.0](https://wiki.wemos.cc/products:d1_mini_shields:motor_shield)) are supported.

Features:
* Fully functional motors speed control. PWM frequency and duty cycle set support;
* Shield can be detected on the I2C bus by standart linux _i2cdetect_ tool;
* Linux driver as a Python module is provided (tested with a Raspberry Pi);
* Custom I2C bootloader. After bootloader installation, shield's firmware can be updated directly from the connected Linux-based board (tested with a Raspberry Pi).

NOTE: as already mentioned [here](https://github.com/pbugalski/wemos_motor_shield/issues/7), with respect to the original firmware, the Motor A directions are inverted (now it is coeherent with pin nomenclature and motor B behaviour).

# Requirements
You need the [ARM toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) and an environment with *make* (on windows you can install [minGW](http://www.mingw.org/)).

# Compiling
Type *make* in the root folder of the project, it compiles the content of "fw" (the main program) and "iap" (the bootloader) folder.

# Flashing
To upload the firmware you need to prepare the hardware and the software environment. Please follow [this guide](https://github.com/thomasfredericks/wemos_motor_shield), but do no execute command to upload the firmware. 

In this repository you can see 2 separated program, the bootloader and the main program. You have to upload them indipendently, placing them in the exact position on the flash, accordingly to *linker script* specification.

    stm32flash.exe -k <Your serial port>
    stm32flash.exe -v -S 0x08000000:0x1000 -w path/to/iap.bin <Your serial port>
    stm32flash.exe -v -S 0x8001000:0x7000 -w path/to/fw.bin <Your serial port>
    
Replace *Your serial port* properly, on windows it will be something like *COM4*.
