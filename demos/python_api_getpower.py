#!/usr/bin/python
from MangDang.mini_pupper.ESP32Interface import ESP32Interface
import time

esp32 = ESP32Interface()

while True:
    print(esp32.get_power_status())
    time.sleep(1 / 20)  # 20 Hz
