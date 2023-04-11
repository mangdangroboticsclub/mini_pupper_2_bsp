#!/bin/bash

cardnums=$(arecord -l  | grep card | cut -d ' ' -f 2 | cut -d ':' -f 1)
arecord -D plughw:$cardnums -d 5 -c2 -r 16000 -f S16_LE -t wav -V stereo -v /tmp/test-mic.wav 
cardnums==$(aplay -l  | grep Headphones | cut -d ' ' -f 2 | cut -d ':' -f 1)
aplay -D plughw:$cardnums /tmp/test-mic.wav
