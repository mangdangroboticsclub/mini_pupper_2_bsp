#!/usr/bin/python
from MangDang.mini_pupper.display import Display, BehaviorState
import time

disp = Display()

disp.show_image('/home/ubuntu/black.1.png')
time.sleep(2)
disp.show_image('/home/ubuntu/sleepBlack.png')
time.sleep(2)
disp.show_image('/home/ubuntu/yellow.2.png')
time.sleep(2)
disp.show_image('/home/ubuntu/sleepYellow.jpg')
time.sleep(2)
disp.show_image('/home/ubuntu/black.2.png')
time.sleep(2)
disp.show_image('/home/ubuntu/yellow.1.png')
time.sleep(2)
disp.show_ip()
