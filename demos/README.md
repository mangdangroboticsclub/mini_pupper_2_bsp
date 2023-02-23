# Demo Scripts

## Installation

Install required Python modules:

sudo pip install -r requirements.txt

## Haptic Demo

Run:

./hapic-demo.py

You can now move the front right leg and the other legs will follow. Note that some legs will move in the oposite direction due to the oriantation of the servos. Correcting this is left as an excercise for the user.

## IMU Demo

Code based on https://github.com/scottshambaugh/mpl_quaternion_views.git

Run:

./quaternion_plotter.py

These script will produce a GUI, either run it when a monitor is connect to your Mini Pupper or use it with ssh X Windows forwarding.

The IMU is positioned "upside down" on the main board, the application must correct the orientation.
