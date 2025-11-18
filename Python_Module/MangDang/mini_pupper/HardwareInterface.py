from MangDang.mini_pupper.Config import ServoParams, PWMParams
import numpy as np
import math

class Joint_checker:
    def __init__(self):
        self.pre_joint_angles = np.zeros((3,4)) 
        self.cnt = 0 
        self.joint_angles_maxLimit = [[55, -55, 55, -55], [-108, -108, -108, -108], [-134, -134, -134, -134]]
        self.joint_angles_minLimit = [[-35, 35, -35, 35], [194, 194, 194, 194], [55, 55, 55, 55]]
        self.deg2rad_arr(self.joint_angles_maxLimit)
        self.deg2rad_arr(self.joint_angles_minLimit)

    def update(self, joint_angles):
        self.pre_joint_angles = np.copy(joint_angles)

    def deg2rad (self, x):
        return x / 180 * math.pi
    
    def rad2deg(self, x):
        return x / math.pi * 180

    def deg2rad_arr (self,joint_angles_limit):
        for i in range(3):
            for j in range(4):
                joint_angles_limit[i][j] = self.deg2rad(joint_angles_limit[i][j])

    def deg_change (self, joint_angles):
        res = np.zeros((3,4))
        tmp_1 = self.pre_joint_angles
        tmp_2 = joint_angles
        for i in range(3):
            for j in range(4):
                res[i][j] = self.rad2deg(tmp_2[i][j]) - self.rad2deg(tmp_1[i][j])
        return res

    def check_limit (self, joint_angles): 
        degchange = self.deg_change(joint_angles)
        #print("Deg change:", degchange[1][0])
        for j in range (4):    #For each leg
            for i in range (3): #For each motor
                if (i == 0) :   #Motor 1 
                    #print("Deg of motor 0 = %d", rad2deg(joint_angles[i][j]))
                    if j % 2 == 0:    #Motor 1 in leg 0 and 2
                        #print("leg 0 and 2")
                        if (degchange[i][j]) > 0 :   #Positive rotation: 
                            #print("Positive rotation")
                            
                            if (joint_angles[i][j] > self.joint_angles_maxLimit[i][j]): 
                                joint_angles[i][j] = self.joint_angles_maxLimit[i][j]
                                print("Blockedddddddddddddddddddddddddd")
                                self.cnt += 1
                        else :       #Negative rotation 
                            #print("Negative rotation")
                            if (joint_angles[i][j] < self.joint_angles_minLimit[i][j]): 
                                joint_angles[i][j] = self.joint_angles_minLimit[i][j]
                                print("Blockedddddddddddddddddddddddddd")
                                self.cnt += 1 

                    else:    #Motor 1 in leg 1 and 3

                        #print("leg 0 and 2")
                        if (degchange[i][j] < 0):    #Positive rotation (maxlimit but < 0)
                            #print("Positive rotation")
                            if (joint_angles[i][j] < self.joint_angles_maxLimit[i][j]):
                                joint_angles[i][j] = self.joint_angles_maxLimit[i][j] 
                                print("Blockedddddddddddddddddddddddddd")
                                self.cnt += 1
                        else :                        #Negative rotation (minlimit but > 0)
                            #print("Negative rotation")
                            if (joint_angles[i][j] > self.joint_angles_minLimit[i][j]):
                                joint_angles[i][j] = self.joint_angles_minLimit[i][j]
                                print("Blockedddddddddddddddddddddddddd")
                                self.cnt += 1
    #####################################################################################################################TESTING DONE 


                elif (i == 1):       #Motor 3 
                    if (degchange[i][j]) < 0 :         #Negative rotation 
                        #Calculate Limit
                        lim = 0
                        constrain_by_motor_2 = joint_angles[2][j] - self.deg2rad(10)
                        if (constrain_by_motor_2 >= 0):     #Positive constrain_by_motor_2
                            lim = constrain_by_motor_2 
                        else :#both of them are negative
                            if (abs(constrain_by_motor_2) < abs(self.joint_angles_maxLimit[i][j])):     
                                lim = constrain_by_motor_2
                            else:
                                lim = self.joint_angles_maxLimit[i][j]
                        #print("Constrain by motor 2:", rad2deg(constrain_by_motor_2))
                        #print("Limit for motor 3:", rad2deg(lim))
                        #Give the constrain for the motor_1
                        if (lim >=0) : 
                            if (joint_angles[i][j] >=0): 
                                if (joint_angles[i][j] < lim) :
                                    joint_angles[i][j] = lim
                                    print("BLOCKKKKKKKKKKKKKKKKKKKKKKK") 
                                    self.cnt += 1 
                            else: 
                                joint_angles[i][j] = lim
                                print("BLOCKKKKKKKKKKKKKKKKKKKKKKK") 
                                self.cnt += 1 
                        
                        else : 
                            if (joint_angles[i][j] < 0): 
                                if (joint_angles[i][j] < lim): 
                                    joint_angles[i][j] = lim
                                    print("BLOCKKKKKKKKKKKKKKKKKKKKKKK")
                                    self.cnt += 1
                        
                    else:
                        
                        lim = 0
                        constrain_by_motor_2 = joint_angles[2][j] + self.deg2rad(194)
                        #print("Constrain by motor 2:", rad2deg(constrain_by_motor_2))

                        if (constrain_by_motor_2 >= self.joint_angles_minLimit[i][j]):     #Positive constrain_by_motor_2
                            lim = self.joint_angles_minLimit[i][j]
                        else :#both of them are negative
                            lim = constrain_by_motor_2
                        
                        #print("Limit for motor 3:", rad2deg(lim))
                        #Give the constrain for the motor_1 
                        if (joint_angles[i][j] >= 0): 
                            if (joint_angles[i][j] > lim) :
                                joint_angles[i][j] = lim
                                print("BLOCKKKKKKKKKKKKKKKKKKKKKKK")
                                self.cnt += 1



                else :                  #Motor 2 
                    #print("Deg of motor 2 = %d", rad2deg(joint_angles[i][j]))
                    if (degchange[i][j] < 0 ): 
                        #if j == 0 : 
                            #print("Neg rotation")
                        if joint_angles[i][j] < self.joint_angles_maxLimit[i][j]:
                            joint_angles[i][j] = self.joint_angles_maxLimit[i][j]
                            print("BLOCKKKKKKKKKKKKKKKKKKKKKKK")
                            self.cnt += 1
                    else:
                        #if j == 0 : 
                            #print("Pos rotation")
                        #Compute Limit
                        lim = 0 
                        constrain_by_motor_3 = joint_angles[1][j] + self.deg2rad(3)
                        if (constrain_by_motor_3 <=0):
                            lim = constrain_by_motor_3
                        else:
                            if (self.joint_angles_minLimit[i][j] >= constrain_by_motor_3):
                                lim = constrain_by_motor_3
                            else :
                                lim = self.joint_angles_minLimit[i][j]
                        #if (j == 0):
                                #print("Limit for motor 2:", rad2deg(lim))
                                #print("Constrain by motor 3:", rad2deg(constrain_by_motor_3))

                        #give constrain for motor 2
                        if (lim >= 0):
                            if (joint_angles[i][j] >= 0):
                                if (joint_angles[i][j] > lim):
                                    joint_angles[i][j] = lim
                                    print("BLOCKKKKKKKKKKKKKKKKKKKKKKK")
                                    self.cnt += 1
                        else : 
                            if (joint_angles[i][j] >= 0):
                                joint_angles[i][j] = lim
                                print("BLOCKKKKKKKKKKKKKKKKKKKKKKK") 
                                self.cnt += 1
                            else :
                                if (joint_angles[i][j] > lim):
                                    joint_angles[i][j] = lim
                                    print("BLOCKKKKKKKKKKKKKKKKKKKKKKK") 
                                    self.cnt += 1 
######################################################################################################################## TESTING DONE


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
