#!/usr/bin/python
import time
import os
import sys
from wemosh import *

names = {MOTOR_A: "Motor A", MOTOR_B: "Motor B"}
dirs = {DIR_BRAKE: "BRAKE", DIR_CCW: "CCW", DIR_CW: "CW", DIR_STOP: "STOP", DIR_STANDBY: "STANDBY"}

def print_motor_state(shield, motor):
    state = shield.motor_get(motor)
    print "%s: [%s %.1f]" %(names[motor], dirs[state[0]], state[1] / 100.0)

def main():
    if len(sys.argv) > 1:
        try:
            address = int(sys.argv[1], 16)
        except ValueError:
            print "Wrong address argument:", sys.argv[1]
            sys.exit()
    else:
        address = 0x30


    shield = WMShield(address)


    if not shield.detect():
        print "No shield (0x%x) found" %(address)
        sys.exit()
    else:
        print "Shield found (0x%x): 0x%x" %(address, shield.get_id())

    try:
        for motor in [MOTOR_A, MOTOR_B]:
            # PWM frequency test
            freq = 80000
            print "Set too high frequency:", freq
            if not shield.freq_set(80000, False):
                print "Freq set error"
            
            freq = shield.freq_get()
            if not freq == MAX_FREQ:
                print "Freq set error, got", freq

            freq = 0
            print "Set max frequency:", MAX_FREQ
            if not shield.freq_set(freq):
                print "Freq set error, got", shield.freq_get()

            # PWM pulse test
            for pulse in range(100 + 1):
                print names[motor], "CW", pulse
                if not shield.motor_set(motor, DIR_CW, pulse):
                    print "Motor set error, got:", shield.motor_get(motor)

                print_motor_state(shield, motor)
                time.sleep(0.01)

            # Stop motor
            print names[motor], "STANDBY"
            if not shield.motor_set(motor, DIR_STANDBY, 0):
                print "Set error, motor:", shield.motor_get(motor)
            
            print_motor_state(shield, motor)

        print "Done"

    except KeyboardInterrupt:
        # Stop all motors
        for motor in [MOTOR_A, MOTOR_B]:
            shield.motor_set(motor, DIR_STANDBY, 0)
        print "Aborted"


if __name__ == '__main__':
    main()