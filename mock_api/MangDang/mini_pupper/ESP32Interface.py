class ESP32Interface:
    """ESP32Interface"""

    def __init__(self):
        self.positions = [512] * 12

    def servos_set_position_torque(self, positions, torque):
        return

    def servos_set_position(self, positions):
        self.positions = positions

    def servos_get_position(self):
        return self.positions

    def servos_get_load(self):
        load = [0.0] * 12
        return load

    def imu_get_data(self):
        imu_data = {"ax": 0.0,
                    "ay": 0.0,
                    "az": 0.0,
                    "gx": 0.0,
                    "gy": 0.0,
                    "gz": 0.0}
        return imu_data

    def get_power_status(self):
        power = {"volt": 8.1,
                 "ampere": 0.3}
        return power
