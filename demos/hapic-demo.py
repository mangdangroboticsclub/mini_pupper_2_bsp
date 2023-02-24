#!/usr/bin/python
from MangDang.mini_pupper.ESP32Interface import ESP32Interface
import time

esp32 = ESP32Interface()
torque = [0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1]

while True:
    positions = esp32.servos_get_position()
    for i in range(3):
        for k in range(3):
            positions[3 + i + 3 * k] = positions[i]
    esp32.servos_set_position_torque(positions, torque)
    time.sleep(1 / 20)  # 20 Hz
