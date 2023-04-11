#!/bin/bash

cardnums==$(aplay -l  | grep Headphones | cut -d ' ' -f 2 | cut -d ':' -f 1)
mpg123 -a hw:$cardnums,0 /home/ubuntu/mini_pupper_bsp/Audio/low_power.mp3
