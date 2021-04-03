#!/bin/env python3

import smbus
import time

bus = smbus.SMBus(1)

# Toggle relay 2 (0x10 is i2c channel)
bus.write_byte_data(0x10, 2, 0xff)
time.sleep(1)
bus.write_byte_data(0x10, 2, 0)
