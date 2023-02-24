#!/usr/bin/python
from MangDang.mini_pupper.ESP32Interface import ESP32Interface
import time

esp32 = ESP32Interface()

while True:
    print(esp32.imu_get_data())
    time.sleep(1 / 20)  # 20 Hz
