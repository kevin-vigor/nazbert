#!/bin/env python3

import smbus
import time

bus = smbus.SMBus(1)

# Toggle relay 2 (0x10 is i2c channel)
bus.write_byte_data(0x10, 1, 0xff)
time.sleep(5)
bus.write_byte_data(0x10, 1, 0)
