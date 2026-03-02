from MangDang.mini_pupper.Config import ServoParams, PWMParams
import numpy as np
import math

class JointChecker:
    """
    Joint angle limiter, implicitly called by HardwareInterface when ENABLE_JOINT_LIMITS is set to True. Checks that the desired joint angles are within the limits specified in min/max, then clip
    Future implementation: can dynamic contraints for velocity/acceleration
    """
    def __init__(self):
        self.cnt_c1 = 0 
        self.cnt_c2 = 0 
        self.cnt_c3 = 0 
        self._joint_max_lim_deg = np.array([
            [55, -55, 55, -55],  # Motor1: abduction
            [-108, -108, -108, -108],  # Motor2: Hip
            [-134, -134, -134, -134],  # Motor3: Knee
        ])
        self._joint_min_lim_deg = np.array([
            [-35, 35, -35, 35],  # Motor1: abduction
            [180, 180, 180, 180],  # Motor2: Hip
            [55, 55, 55, 55],  # Motor3: Knee
        ])
        self._joint_max_lim = np.deg2rad(self._joint_max_lim_deg)
        self._joint_min_lim = np.deg2rad(self._joint_min_lim_deg)
        self.upper_bound = np.maximum(self.joint_max_lim, self.joint_min_lim)
        self.lower_bound = np.minimum(self.joint_max_lim, self.joint_min_lim)

    def check_limit(self, joint_angles):
        """
        Check and make sure the joint angles are within limit specified.
        """
        # Constraints 1: Static constraints of each motor range
        clipped_angles = np.clip(joint_angles, self.lower_bound, self.upper_bound) 
        n_clipped_motor = np.sum(clipped_angles != joint_angles)
        self.cnt_c1 += n_clipped_motor

        # Constraints 2: Dynamic coupling of Motor 2 and 3 (Motor 2 max <= Motor 3 + 194°)
        motor2_angles = clipped_angles[1, :] # Use the already clipped angles as the base
        motor3_angles = clipped_angles[2, :]
        motor2_max = motor3_angles + np.deg2rad(194)
        clipped_angles_c2 = np.minimum(motor2_angles, motor2_max)
        # Count additional clipping from constraint 2 before updating
        self.cnt_c2 += np.sum(clipped_angles_c2 != clipped_angles[1, :])
        clipped_angles[1, :] = clipped_angles_c2  # Update clipped_angles, assuming constraint 2 is smaller than constraint 1

        # Constraints 3: Dynamic coupling of Motor 2 and 3 (Motor 3 max <= Motor 2 + 3°)
        motor3_max = motor2_angles + np.deg2rad(3)
        clipped_angles_c3 = np.minimum(motor3_angles, motor3_max)
        # Count additional clipping from constraint 3 before updating
        self.cnt_c3 += np.sum(clipped_angles_c3 != clipped_angles[2, :])
        clipped_angles[2, :] = clipped_angles_c3  # Update clipped_angles, assuming constraint 3 is smaller than constraint 1

        #print(f"Clipped times: {self.cnt_c1}, {self.cnt_c2}, {self.cnt_c3}")
        #print(np.degrees(clipped_angles))
        return clipped_angles

    @property
    def joint_max_lim(self):
        return self._joint_max_lim
    @property
    def joint_min_lim(self):
        return self._joint_min_lim


class HardwareInterface:
    """Hardware layer for commanding servo joints.

    By default, joint limits are enabled and all multi-joint commands
    are passed through JointChecker to enforce static and dynamic
    constraints. Single-joint commands are provided as a lower-level
    interface and do not perform any safety checks.
    """

    def __init__(self, joint_checker_flag=True):
        self.pwm_params = PWMParams()
        self.servo_params = ServoParams()
        self.ENABLE_JOINT_LIMITS = joint_checker_flag
        if self.ENABLE_JOINT_LIMITS:
            print("Joint limits initialized.")
            self.joint_checker = JointChecker()
        else:
            print("Joint limits not initialized.")

    def set_actuator_postions(self, joint_angles):
        """Set all joint angles (3x4) for the four legs.

        When ENABLE_JOINT_LIMITS is True, the joint_angles matrix is
        first passed through JointChecker.check_limit().
        This is the recommended, safety-aware API.
        """
        if self.ENABLE_JOINT_LIMITS:
            send_servo_commands(self.pwm_params, self.servo_params, self.joint_checker.check_limit(joint_angles))
        else:
            send_servo_commands(self.pwm_params, self.servo_params, joint_angles)

    def set_actuator_position(self, joint_angle, axis, leg):
        """Command a single joint on a single leg with no limit checks.

        This API bypasses JointChecker and does not enforce static or dynamic joint constraints. 
        It should only be used for advanced tasks such as calibration or debugging.
        For normal operation, prefer set_actuator_postions().
        """
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
    servo_position = (servo_params.neutral_position - servo_params.micros_per_rad * angle_deviation)
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
