from MangDang.mini_pupper.Config import ServoParams, PWMParams
import numpy as np


class HardwareInterface:
    def __init__(self):
        self.pwm_params = PWMParams()
        self.servo_params = ServoParams()

    def set_actuator_postions(self, joint_angles):
        send_servo_commands(self.pwm_params, self.servo_params, joint_angles)

    def set_actuator_position(self, joint_angle, axis, leg):
        send_servo_command(self.pwm_params, self.servo_params, joint_angle, axis, leg)


def angle_to_position(angle, servo_params, axis_index, leg_index):
    """Converts a desired servo angle into the corresponding position command

    Parameters
    ----------
    angle : float
        Desired servo angle, relative to the vertical (z) axis
    servo_params : ServoParams
        ServoParams object
    axis_index : int
        Specifies which joint of leg to control. 0 is abduction servo, 1 is inner hip servo, 2 is outer hip servo.
    leg_index : int
        Specifies which leg to control. 0 is front-right, 1 is front-left, 2 is back-right, 3 is back-left.

    Returns
    -------
    float
        desired servo position
    """
    angle_deviation = (angle - servo_params.neutral_angles[axis_index, leg_index]) * servo_params.servo_multipliers[axis_index, leg_index]
    servo_position = (servo_params.neutral_position + servo_params.micros_per_rad * angle_deviation)
    return servo_position


def angle_to_servo_position(angle, pwm_params, servo_params, axis_index, leg_index):
    servo_position_f = angle_to_position(angle, servo_params, axis_index, leg_index)
    if np.isnan(servo_position_f):
        return 0
    return int(servo_position_f)


def send_servo_commands(pwm_params, servo_params, joint_angles):
    positions = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    for leg_index in range(4):
        for axis_index in range(3):
            servo_position = angle_to_servo_position(
                joint_angles[axis_index, leg_index],
                pwm_params,
                servo_params,
                axis_index,
                leg_index,
            )
            positions[pwm_params.servo_ids[axis_index, leg_index] - 1] = servo_position
    pwm_params.esp32.servos_set_position(positions)


def send_servo_command(pwm_params, servo_params, joint_angle, axis, leg):
    # not implemented
    pass


def deactivate_servos(pi, pwm_params):
    for leg_index in range(4):
        for axis_index in range(3):
            pi.set_pwm(pwm_params.servo_ids[axis_index, leg_index], 0, 0)
