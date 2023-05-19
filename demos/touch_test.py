#!/usr/bin/env python3
#
# Copyright 2023 MangDang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Description: Mini Pupper touch pannel test script.
#
import time
import RPi.GPIO as GPIO

# There are 4 areas for touch actions
# Each GPIO to each touch area
touchPin_Front = 6
touchPin_Left  = 3
touchPin_Right = 16
touchPin_Back  = 2

# Use GPIO number but not PIN number
GPIO.setmode(GPIO.BCM)

# Set up GPIO numbers to input
GPIO.setup(touchPin_Front, GPIO.IN)
GPIO.setup(touchPin_Left,  GPIO.IN)
GPIO.setup(touchPin_Right, GPIO.IN)
GPIO.setup(touchPin_Back,  GPIO.IN)

# Detection Loop
while True:
    touchValue_Front = GPIO.input(touchPin_Front)
    touchValue_Back  = GPIO.input(touchPin_Back)
    touchValue_Left  = GPIO.input(touchPin_Left)
    touchValue_Right = GPIO.input(touchPin_Right)

    print("Touch Detect Front, Back, Right, Left: %d, %d, %d, %d" %(touchValue_Front, touchValue_Back, touchValue_Right, touchValue_Left))
    time.sleep(0.5)
