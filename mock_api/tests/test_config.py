import filecmp
import os
from MangDang.mini_pupper.Config import Configuration
import numpy as np


def test_get_configuration():
    config = Configuration()
    assert config.ps4_color == {'red': 0, 'blue': 0, 'green': 255}
