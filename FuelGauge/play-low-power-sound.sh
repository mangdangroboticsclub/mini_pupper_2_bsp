#!/bin/bash

# Try to use dedicated Headphones device if available, fallback to default
if aplay -l 2>/dev/null | grep -q Headphones; then
    cardnums=$(aplay -l | grep Headphones | cut -d ' ' -f 2 | cut -d ':' -f 1)
    mpg123 -a hw:$cardnums,0 /home/ubuntu/mini_pupper_bsp/Audio/low_power.mp3
else
    # Fallback for systems without dedicated Headphones device
    mpg123 /home/ubuntu/mini_pupper_bsp/Audio/low_power.mp3
fi
