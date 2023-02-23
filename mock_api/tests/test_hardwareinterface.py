import filecmp
import os
from MangDang.mini_pupper.HardwareInterface import HardwareInterface
import numpy as np


def test_set_actuator_positions():
    data = np.array([[0, 0, 0, 0], [45, 45, 45, 45], [-45, -45, -45, -45]])
    os.system("rm -f /tmp/HardwareInterface.log")
    hardware_interface = HardwareInterface()
    hardware_interface.set_actuator_postions(np.radians(data))
    assert filecmp.cmp(os.path.join(os.path.dirname(__file__), 'expected_results', 'hardwareinterface_1'),
                       '/tmp/HardwareInterface.log')
