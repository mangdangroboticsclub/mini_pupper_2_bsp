import MangDang.mini_pupper.nvram as nvram
import numpy as np


def test_no_nvmem_file():
    nvram.delete()
    try:
        nvram.read()
        assert False
    except Exception:
        assert True


def test_write_read():
    MICROS_PER_RAD = 11.111 * 180.0 / np.pi
    Matrix_EEPROM = np.array([[0, 0, 0, 0],
                              [45, 45, 45, 45],
                              [-45, -45, -45, -45]])
    data = {'MICROS_PER_RAD': MICROS_PER_RAD,
            'NEUTRAL_ANGLE_DEGREES': Matrix_EEPROM}
    nvram.write(data)
    read_data = nvram.read()
    assert data.keys() == read_data.keys()
    assert data['MICROS_PER_RAD'] == read_data['MICROS_PER_RAD']
    np.testing.assert_equal(data['NEUTRAL_ANGLE_DEGREES'], read_data['NEUTRAL_ANGLE_DEGREES'])
