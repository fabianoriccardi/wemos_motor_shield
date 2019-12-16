# Wemos Motor Shield
Alternative firmware for a popular motor shield from Wemos Electronics.

Both HW revisions ([v1.0.0](https://wiki.wemos.cc/products:retired:motor_shield_v1.0.0) and [v2.0.0](https://wiki.wemos.cc/products:d1_mini_shields:motor_shield)) are supported.

Features:
* Fully functional motors speed control. PWM frequency and duty cycle set support;
* Shield can be detected on the I2C bus by standart linux _i2cdetect_ tool;
* Linux driver as a Python module is provided (tested with a Raspberry Pi);
* Custom I2C bootloader. After bootloader installation, shield's firmware can be updated directly from the connected Linux-based board (tested with a Raspberry Pi).
