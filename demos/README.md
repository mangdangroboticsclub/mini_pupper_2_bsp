# Demo Guide

## Haptic Demo

Run:

./haptic-demo.py

You can now move the front right leg and the other legs will follow. Note that some legs will move in the oposite direction due to the oriantation of the servos. Correcting this is left as an excercise for the user.

## IMU Demo

Code based on https://github.com/scottshambaugh/mpl_quaternion_views.git

Run:
```
cd ~/mini_pupper_bsp/demos
# Install required Python modules:
sudo pip install -r requirements.txt
./quaternion_plotter.py
```

These script will produce a GUI, either run it when a monitor is connect to your Mini Pupper or use it with ssh X Windows forwarding.

The IMU is positioned "upside down" on the main board, the application must correct the orientation.

## Audio Demo

Make sure there is no HDMI calbe connect Mini Pupper and Run:

./audio_test.py

you have 5 seconds to record your voice, after that the recording will be replayed.
For the detailed info, please see the script code.

## Camera Demo

Connect your MIPI camera module to Mini Pupper, and install following the below link,
https://github.com/mangdangroboticsclub/mini_pupper_2_bsp/tree/main/RPiCamera

After that, run:

./camera_ffplay_test.py
