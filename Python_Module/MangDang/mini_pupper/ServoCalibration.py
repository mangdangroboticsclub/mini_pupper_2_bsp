# Mini Pupper V2 stores calibration data in ESP32

import numpy as np


MICROS_PER_RAD = (760 - 210) / np.pi
NEUTRAL_ANGLE_DEGREES = np.array(
[[    0.,    0.,    0.,    0.],
 [   45.,   45.,   45.,   45.],
 [  -45.,  -45.,  -45.,  -45.]]
)
