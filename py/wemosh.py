import smbus
import struct
import time
import os
import sys
import filecmp
from progress.bar import Bar

IAP = 0
FW = 1

MOTOR_A = 0
MOTOR_B = 1

DIR_BRAKE = 0
DIR_CCW = 1
DIR_CW = 2
DIR_STOP = 3
DIR_STANDBY = 4

MAX_FREQ = 31250

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
            # print "Read error", 
            # if (cmd != None):
            #     print "(cmd: 0x%x)" % (cmd)
            # else:
            #     print "(cmd: None)"
            pass

    def __write(self, cmd, data):
        try:
            self.bus.write_i2c_block_data(self.addr, cmd, data)
            return True
        except IOError:
            # print "Write error (cmd: 0x%x)" % (cmd)
            return False

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
        if not self.__write(cmd, data):
            return False
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
        data = list(bytearray(struct.pack(">I", freq)))
        cmd = int(0x00 | (data[0] & 0x0F))
        if not self.__write(cmd, data[1:]):
            return False
        # Frequency is not set precisely due to prescaler discreteness, so allow 1% accuracy.
        if verify:
            answ = self.freq_get()
            if answ == None:
                return False
            # 0 - set max frequency.
            if freq != 0:
                accuracy = abs(1 - answ / float(freq))
                if accuracy > 1:
                    return False
            elif freq == 0 and answ != MAX_FREQ:
                return False
        return True

    def freq_get(self):
        answ = self.__read(0x00, 4)
        if answ != None:
            data = struct.unpack(">I", bytearray(answ))[0]
            return data

    def boot(self, mode, verify = True):
        self.__write(0x20, [mode])
        if verify:
            time.sleep(0.50)
            if not (mode == self.get_id()):
                return False
        return True

    def get_id(self):
        answ = self.__read(0x40, 1)
        if answ != None:
            return answ[0]

    def detect(self):
        return (self.__read() == self.addr)

    def upload(self, pkt_num, data):
        cmd = int(0x30 | (pkt_num >> 8))
        pkt_num = int(pkt_num & 0xFF)
        return self.__write(cmd, [pkt_num] + data)

    def get_fwlen(self):
        answ = self.__read(0x50, 4)
        if answ != None:
            return struct.unpack(">I", bytearray(answ))[0]

    def set_fwlen(self, length, verify = True):
        print list(bytearray(struct.pack(">I", length)))
        self.__write(0x50, list(bytearray(struct.pack(">I", length))))
        if verify:
            if not (length == self.get_fwlen()):
                return False
        return True

    def read(self, length = 31):
        cmd = 0x30
        answ = self.__read(cmd, length)
        return answ

    def read_fw(self, size, path = None):
        fw = []
        pkt_len = 30
        pkts_count = size / pkt_len

        with Bar('Reading', max = pkts_count) as bar:
            for i in range(pkts_count):
                res = self.read(pkt_len)
                if res != None and len(res) > 0:
                    fw += res
                    bar.next()
                else:
                    print "Error"
                    print "Total bytes:", len(fw)
                    return None

            tail = size % pkt_len
            if (tail > 0):
                res = self.read(tail)
                if res != None and len(res) > 0:
                    fw += res
                else:
                    print "Error"
                    print "Total bytes:", len(fw)
                    return None

        if path != None:
            with open(path, "wb") as f:
                f.write(bytearray(fw))

        # print "fw:", ' '.join('{:02X}'.format(x) for x in fw)
        return fw

    def write_fw(self, path, verify = True):
        with open(path, "rb") as f:
            size = os.path.getsize(path)
            print "File size: %d bytes" % (size)
            pkt_len = 30
            pkt_num = size / pkt_len
            if size % pkt_len > 0:
                pkt_num += 1

            with Bar('Writing', max = pkt_num) as bar:

                while size > 0:
                    data = f.read(pkt_len)
                    pkt_num -= 1

                    timeout = 3
                    while ((not self.upload(pkt_num, list(bytearray(data)))) and timeout > 0):
                        # Waiting for packet is writing
                        time.sleep(0.1)
                        timeout -= 1
                        if pkt_num == 0:
                            break

                    if not timeout:
                        return False
                    
                    bar.next()
                    size -= len(data)
                
                # Waiting last packet is writing
                time.sleep(0.1)

        if verify:
            print "Verification..."
            size = os.path.getsize(path)
            if len(self.read_fw(size, "read.bin")) != size:
                print "Verification failed: size missmatch"
                return False
            if not filecmp.cmp(path, "read.bin"):
                print "Verification failed: content missmatch"
                return False
            else:
                print "FW successfully written"

        return True