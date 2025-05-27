#!/usr/bin/python
import sys
sys.path.append('../Python_Module/MangDang/mini_pupper/')

from ESP32Interface import ESP32Interface
import time

# initial middle positions
positions = [512] * 12

servos = [2, 5, 8, 11]  
signs = [1, -1, 1, -1]  
max_delta = 100           


esp32 = ESP32Interface()

delta = 1
upper_max = 512 + max_delta
lower_min = 512 - max_delta

for j in range(1000):
    esp32.servos_set_position(positions)
    
    for i in range(len(servos)):
        servo_idx = servos[i] - 1
        new_pos = positions[servo_idx] + delta * signs[i]
        
        if new_pos > upper_max:
            new_pos = upper_max
        elif new_pos < lower_min:
            new_pos = lower_min
        positions[servo_idx] = new_pos

    if positions[servos[0]-1] >= upper_max or positions[servos[0]-1] <= lower_min:
        delta *= -1
    
    time.sleep(1/100)

# which servos to move: count servos from 1 to 12
servos = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
# maximum deviation from neutral position
max_delta = 50

# move only hips
#servos = [1, 4, 7, 10]

# move only upper legs
#servos = [2, 5, 8, 11]

# move only lower legs
#servos = [3, 6, 9, 12]

esp32 = ESP32Interface()

delta = 1
upper_max = min(512 + max_delta, 1023)
lower_min = max(512 - max_delta, 0)
while True:
    esp32.servos_set_position(positions)
    for servo in servos:
        positions[servo - 1] += delta
        positions[servo - 1] %= 1024
    # limit to configured maximum deviation
    if positions[servos[0] - 1] >= upper_max or positions[servos[0] - 1] <= lower_min:
        delta *= -1
    time.sleep(1 / 200)  # 500 Hz