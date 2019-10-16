import smbus
import struct


MOTOR_A = 0
MOTOR_B = 1

DIR_BRAKE = 0
DIR_CCW = 1
DIR_CW = 2
DIR_STOP = 3
DIR_STANDBY = 4


class WMShield():
    def __init__(self, addr, bus_num = 1):
        self.addr = addr
        self.bus = smbus.SMBus(bus_num)

    def __read(self, cmd = None, length = 0):
        try:
            if (cmd == None):
                return self.bus.read_byte(self.addr)
            else:
                return self.bus.read_i2c_block_data(self.addr, cmd, length)
        except IOError:
            print "Read error"

    def __write(self, cmd, data):
        try:
            return self.bus.write_i2c_block_data(self.addr, cmd, data)
        except IOError:
            print "Write error"

    def __parce_motor(self, data):
        return [data[0]] + list(struct.unpack(">H", bytearray(data[1:3])))

    def get_address():
        return self.addr

    def motor_set(self, motor, direction, pulse, verify = True):
        if (motor == MOTOR_A):
            cmd = 0x10
        elif (motor == MOTOR_B):
            cmd = 0x11
        else:
            return False
        data = [direction] + list(bytearray(struct.pack(">H", pulse * 100)))
        self.__write(cmd, data)
        if verify:
            answ = self.motor_get()
            if answ == None:
                return False
            m1 = self.motor_get()[0:2]
            m2 = self.motor_get()[2:4]
            # Percentage is not set precisely due to PWM steps discreteness, so allow 0.5% accuracy.
            if motor == MOTOR_A and (m1[0] != direction or abs(m1[1] / 100.0 - pulse) > 0.5):
                return False
            if motor == MOTOR_B and (m2[0] != direction or abs(m2[1] / 100.0 - pulse) > 0.5):
                return False
        return True

    def motor_get(self, motor = None):
        answ = self.__read(0x10, 6)
        if answ != None:
            if (motor == MOTOR_A):
                return self.__parce_motor(answ[0:3])
            if (motor == MOTOR_B):
                return self.__parce_motor(answ[3:6])
            else:
                return self.__parce_motor(answ[0:3]) + self.__parce_motor(answ[3:6])

    def freq_set(self, freq, verify = True):
        self.__write(0x00, list(bytearray(struct.pack("<H", freq))))
        if verify:
            if self.freq_get() != freq:
                return False
        return True

    def freq_get(self):
        answ = self.__read(0x00, 2)
        if answ != None:
            return struct.unpack("<H", bytearray(answ))[0]

    def detect(self):
        return (self.__read() == self.addr)