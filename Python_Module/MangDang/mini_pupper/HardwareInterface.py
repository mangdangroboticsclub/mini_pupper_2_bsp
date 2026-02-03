from MangDang.mini_pupper.Config import ServoParams, PWMParams, ENABLE_JOINT_LIMITS
import numpy as np
import math


def deg2rad(x):
    """Convert degrees to radians"""
    return x / 180 * math.pi


class Joint_checker:
    """
    Joint angle limit checker to protect servos from damage
    """
    def __init__(self):
        self.cnt = 0  # Counter for how many times limits were hit
        self.enabled = ENABLE_JOINT_LIMITS  # 从全局配置读取开关状态
        
        # Define joint angle limits in radians
        # Row 0: Motor 1 (Hip) - left/right movement
        # Row 1: Motor 2 (Upper leg) - backward/forward movement  
        # Row 2: Motor 3 (Lower leg) - bending movement
        # Columns: [FR, FL, BR, BL] (Front Right, Front Left, Back Right, Back Left)
        
        # Maximum limits (clockwise rotation limits)
        self.joint_angles_maxLimit = np.array([
            [deg2rad(55), deg2rad(-55), deg2rad(55), deg2rad(-55)],      # Motor 1 max
            [deg2rad(-108), deg2rad(-108), deg2rad(-108), deg2rad(-108)],  # Motor 2 max
            [deg2rad(-134), deg2rad(-134), deg2rad(-134), deg2rad(-134)]   # Motor 3 max
        ])
        
        # Minimum limits (counter-clockwise rotation limits)
        # Note: Motor 2 min changed from 194 to 180 to prevent motor jam/stuck
        self.joint_angles_minLimit = np.array([
            [deg2rad(-35), deg2rad(35), deg2rad(-35), deg2rad(35)],      # Motor 1 min
            [deg2rad(180), deg2rad(180), deg2rad(180), deg2rad(180)],    # Motor 2 min (was 194, causes jam)
            [deg2rad(55), deg2rad(55), deg2rad(55), deg2rad(55)]         # Motor 3 min
        ])
        
        # Store previous state
        self.pre_state_joint_angles = np.zeros((3, 4))
    
    def check_limit(self, joint_angles):
        """
        Check and enforce joint angle limits
        Modifies joint_angles in-place to constrain within safe limits
        
        Parameters:
        -----------
        joint_angles : numpy.ndarray
            3x4 array of joint angles [motor_id, leg_id]
        """
        # 如果限制被禁用，直接返回不做任何检查
        if not self.enabled:
            return
        
        for motor_idx in range(3):
            for leg_idx in range(4):
                original_angle = joint_angles[motor_idx][leg_idx]
                
                # Check max limit (note: some motors have inverted limits)
                if motor_idx == 0:  # Motor 1 (Hip)
                    if leg_idx in [0, 2]:  # FR, BR
                        if joint_angles[motor_idx][leg_idx] > self.joint_angles_maxLimit[motor_idx][leg_idx]:
                            joint_angles[motor_idx][leg_idx] = self.joint_angles_maxLimit[motor_idx][leg_idx]
                            self.cnt += 1
                        elif joint_angles[motor_idx][leg_idx] < self.joint_angles_minLimit[motor_idx][leg_idx]:
                            joint_angles[motor_idx][leg_idx] = self.joint_angles_minLimit[motor_idx][leg_idx]
                            self.cnt += 1
                    else:  # FL, BL
                        if joint_angles[motor_idx][leg_idx] < self.joint_angles_maxLimit[motor_idx][leg_idx]:
                            joint_angles[motor_idx][leg_idx] = self.joint_angles_maxLimit[motor_idx][leg_idx]
                            self.cnt += 1
                        elif joint_angles[motor_idx][leg_idx] > self.joint_angles_minLimit[motor_idx][leg_idx]:
                            joint_angles[motor_idx][leg_idx] = self.joint_angles_minLimit[motor_idx][leg_idx]
                            self.cnt += 1
                
                elif motor_idx == 1:  # Motor 2 (Upper leg)
                    # Motor 2: max is negative, min is positive (inverted)
                    if joint_angles[motor_idx][leg_idx] < self.joint_angles_maxLimit[motor_idx][leg_idx]:
                        joint_angles[motor_idx][leg_idx] = self.joint_angles_maxLimit[motor_idx][leg_idx]
                        self.cnt += 1
                    elif joint_angles[motor_idx][leg_idx] > self.joint_angles_minLimit[motor_idx][leg_idx]:
                        joint_angles[motor_idx][leg_idx] = self.joint_angles_minLimit[motor_idx][leg_idx]
                        self.cnt += 1
                
                elif motor_idx == 2:  # Motor 3 (Lower leg)
                    # Motor 3: max is negative, min is positive (inverted)
                    if joint_angles[motor_idx][leg_idx] < self.joint_angles_maxLimit[motor_idx][leg_idx]:
                        joint_angles[motor_idx][leg_idx] = self.joint_angles_maxLimit[motor_idx][leg_idx]
                        self.cnt += 1
                    elif joint_angles[motor_idx][leg_idx] > self.joint_angles_minLimit[motor_idx][leg_idx]:
                        joint_angles[motor_idx][leg_idx] = self.joint_angles_minLimit[motor_idx][leg_idx]
                        self.cnt += 1
        
        # Apply dynamic coupling constraints between Motor 2 and Motor 3
        # These constraints prevent mechanical interference between upper and lower leg
        for leg_idx in range(4):
            motor2_angle = joint_angles[1][leg_idx]  # Upper leg
            motor3_angle = joint_angles[2][leg_idx]  # Lower leg
            
            # Constraint 1: Motor 2 max <= Motor 3 + 194°
            # (Upper leg limited by lower leg position)
            motor2_dynamic_max = motor3_angle + deg2rad(194)
            if motor2_angle > motor2_dynamic_max:
                joint_angles[1][leg_idx] = motor2_dynamic_max
                self.cnt += 1
            
            # Constraint 2: Motor 3 max <= Motor 2 + 3°
            # (Lower leg limited by upper leg position)
            motor3_dynamic_max = motor2_angle + deg2rad(3)
            if motor3_angle > motor3_dynamic_max:
                joint_angles[2][leg_idx] = motor3_dynamic_max
                self.cnt += 1
    
    def update(self, joint_angles):
        """
        Update the stored previous state
        
        Parameters:
        -----------
        joint_angles : numpy.ndarray
            3x4 array of current joint angles
        """
        self.pre_state_joint_angles = np.copy(joint_angles)


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
