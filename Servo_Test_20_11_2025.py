import numpy as np
import time
import math
from src.IMU import IMU
from src.Controller import Controller
from src.JoystickInterface import JoystickInterface
from src.State import BehaviorState, State
from MangDang.mini_pupper.HardwareInterface import HardwareInterface
from MangDang.mini_pupper.Config import Configuration
from pupper.Kinematics import four_legs_inverse_kinematics
from MangDang.mini_pupper.display import Display
from src.MovementScheme import MovementScheme
from src.createDanceActionListSample import MovementLib
from src.Command import Command
from MangDang.mini_pupper.HardwareInterface import Joint_checker

def deg2rad (x):
    return x / 180 * math.pi
def rad2deg(x):
    return x / math.pi * 180

def main(use_imu=False):
    """Main program
    """

    # Create config
    config = Configuration()
    hardware_interface = HardwareInterface()
    disp = Display()
    disp.show_ip()
    joint_checker = Joint_checker()
    # Create imu handle
    if use_imu:
        imu = IMU(port="/dev/ttyACM0")
        imu.flush_buffer()

    # Create controller and user input handles
    controller = Controller(
        config,
        four_legs_inverse_kinematics,
    )
    state = State()
    
    #manage the previous angle stat
    #pre_state
    pre_state_joint_angles = np.copy(state.joint_angles)
    

    print("Creating joystick listener...")
    joystick_interface = JoystickInterface(config)
    print("Done.")

    #Create movement group scheme instance and set a default false state
    movementCtl = MovementScheme(MovementLib)
    dance_active_state = False

    last_loop = time.time()

    print("Summary of gait parameters:")
    print("overlap time: ", config.overlap_time)
    print("swing time: ", config.swing_time)
    print("z clearance: ", config.z_clearance)
    print("x shift: ", config.x_shift)
    
    # Limit the angle of each servo motor based on the current structure to protect the servo motors.
    #joint_angles_maxLimit = [[1.2,  0.6, 1,  0.5], [1.3, 1.3, 1.6, 1.6], [0.7,  0.7,   0,    0]]
    #joint_angles_minLimit = [[-0.5, -1, -0.6, -1], [0,  0,  -0.6, -0.6], [-1.5, -1.5, -1.2, -1.2]]

    #join_angles_maxLimit = [[55, -55, 55, -55], [-108, -108, -108, -108], [-134, -134, -134, -134]]

    #This is the direction that the motor 1 rotate in + angle, motor 2 rotate in - angle, motor 3 rotate in - angle
    #Motor 3 depends on motor 2

    #deg2rad_arr(joint_angles_maxLimit)

    #Working on 
    #joint_angle_minLimit = [[-35, 35, -35, 35], [194, 194, 194, 194], [55, 55, 55, 55]]
    #This is the direction that the motor 1 rotate in - angle, motor 2 rotate in + angle, motor 3 rotate in + angle
    #Motor 2 depends on motor 3
    #Motor 3 is trapped at some point until motor 2 continue to move 
    #Motor 2 is trapped my motor 3 and its own limit
    #If motor 2 reach its own limit, motor 3 can continue to move until its own limit
    #Motor 2 constrained = motor 3 + 3
    #Motor 3 constrained = motor 2 + 194
    #deg2rad_arr(joint_angles_minLimit)

    #print(joint_angles_maxLimit)
    #print(joint_angles_minLimit)
    """
    2   1
    4   3
    hip -> leg1 -> leg2
    #In deg
    #+ is clockwise, - is counterclockwise
    #Servo 0 is place right position 
    #Servo 1 is place invesed position 
    #Servo 2 is place right position
    joint_angles_maxLimit = [[55, -55, 55, -55]], [-108, -108, -108, -108], [-134, -134, -134, -134]]
    joint_angle_minLimit = [[], [], []]
    #------------------------------------>   Also the max of leg1 is depent on the leg22 according to the following function : 
    #                                        If Leg1_max > (No release of freedom)   ->   leg2_max = leg1_max - 10  (for leg1) for angle of leg1 negative
    ""          "                            check if abs(leg1_max (itself limitation)) > abs(leg1_max - 10)      -> leg1_max = leg2_max - 10
    //First row : hip 
    //Second row :leg2
    //Third row : leg1
    """
    


###Angle Testing phase
    """
    state.joint_angles = np.array([
        [-0.00644456,  0.00644456, -0.00644456,  0.00644456],
        [ 0.88270319,  0.88270319,  0.88270319,  0.88270319],
        [-0.69934338, -0.69934338, -0.69934338, -0.69934338]
    ])
    #-50 for 0
    #print(int(deg2rad(state.joint_angles[2][0])))
    hardware_interface.set_actuator_postions(state.joint_angles)
    motor_3 = int (rad2deg(state.joint_angles[1][0]))
    motor_2 = int (rad2deg(state.joint_angles[2][0]))
    motor_1 = int (rad2deg(state.joint_angles[0][0]))
    motor_3 -= 10
    #motor_2 = 194
    #motor_1 = 50
    #motor_1 = motor_2 + 1 
    #motor_2 = 147
    #motor_2 = motor_1 + 194
    #temp = -134
    #angle = abs(-134) - abs(temp) + int (rad2deg(state.joint_angles[2][1]))
    #print("Initial angle1:", angle)
    print("Motor 3:", motor_3)
    while True:
        now = time.time()
        if now - last_loop < 0.5:
            continue
        last_loop = time.time()
        
        
        #print(deg2rad(angle))
        #state.joint_angles[1][0] = deg2rad(angle2-10)
        #Angle2 is angle of leg 1 
        #Angle1 is angle of leg 2 
        #state.joint_angles[2][0] = deg2rad(angle2)
        #angle1 50
        #angle2 -40     -> angle1 = angle2 + 90 
        #state.joint_angles[2][0] = deg2rad(angle2)
        #state.joint_angles[1][1] = deg2rad(angle1)
        #state.joint_angles[1][0] = deg2rad(angle1)
        #state.joint_angles[2][0] = deg2rad(-134)
        #state.joint_angles[2][1] = deg2rad(-134)
        #print(angle2)
        #print(angle1)
        #print(state.joint_angles)
        #state.joint_angles[1][0] = deg2rad(motor_2)
        #state.joint_angles[2][0] = deg2rad(motor_2)
        #state.joint_angles[1][0] = deg2rad(motor_3)
        #print(state.joint_angles)
        #print("-------------------------------------------------------------------")
        #print(pre_state_joint_angles)
        #Check the limit of each motor angle
        state.joint_angles[0][0] = deg2rad(motor_1)
        state.joint_angles[0][2] = deg2rad(motor_1)
        state.joint_angles[0][1] = deg2rad(-motor_1)
        state.joint_angles[0][3] = deg2rad(-motor_1)

        joint_checker.check_limit (state.joint_angles)

        # Update the pwm widths going to the servos
        hardware_interface.set_actuator_postions(state.joint_angles)

        #Update the previous joint angle state
        joint_checker.update(state.joint_angles)

        motor_1 +=2
        #print("angle1", angle1)
        #print("angle2", angle2)
        #angle1 -= 1 
        #angle2 -= 1
        #temp -= 2
        #angle1 -= 2
        #angle1 += 1 
    """

    


    ######################3Motor2 Testing
    #Anticlockwise rotation testing 
    state.joint_angles = np.array([
            [-0.00644456,  0.00644456, -0.00644456,  0.00644456],
            [ 0.88270319,  0.88270319,  0.88270319,  0.88270319],
            [-0.69934338, -0.69934338, -0.69934338, -0.69934338]
        ])
        #-50 for 0
        #print(int(deg2rad(state.joint_angles[2][0])))
    hardware_interface.set_actuator_postions(state.joint_angles)
    motor_3 = int (rad2deg(state.joint_angles[1][0]))
    motor_2 = int (rad2deg(state.joint_angles[2][0]))
    motor_1 = int (rad2deg(state.joint_angles[0][0]))
    motor_3 -= 10
    #motor_2 = 194
    #motor_1 = 50
    #motor_1 = motor_2 + 1 
    #motor_2 = 147
    #motor_2 = motor_1 + 194
    #temp = -134
    #angle = abs(-134) - abs(temp) + int (rad2deg(state.joint_angles[2][1]))
    #print("Initial angle1:", angle)
    print("Motor 3:", motor_3)
    while True:
        now = time.time()
        if now - last_loop < 0.5:
            continue
        last_loop = time.time()
        
        
        ###############################################################################Anticlockwise testing
        #print(deg2rad(angle))
        #state.joint_angles[1][0] = deg2rad(angle2-10)
        #Angle2 is angle of leg 1 
        #Angle1 is angle of leg 2 
        #state.joint_angles[2][0] = deg2rad(angle2)
        #angle1 50
        #angle2 -40     -> angle1 = angle2 + 90 
        #state.joint_angles[2][0] = deg2rad(angle2)
        #state.joint_angles[1][1] = deg2rad(angle1)
        #state.joint_angles[1][0] = deg2rad(angle1)
        #state.joint_angles[2][0] = deg2rad(-134)
        #state.joint_angles[2][1] = deg2rad(-134)
        #print(angle2)
        #print(angle1)
        #print(state.joint_angles)
        #state.joint_angles[1][0] = deg2rad(motor_2)
        #state.joint_angles[2][0] = deg2rad(motor_2)
        #state.joint_angles[1][0] = deg2rad(motor_3)
        #print(state.joint_angles)
        #print("-------------------------------------------------------------------")
        #print(pre_state_joint_angles)
        #Check the limit of each motor angle
        #state.joint_angles[0][0] = deg2rad(motor_1)
        #state.joint_angles[0][2] = deg2rad(motor_1)
        #state.joint_angles[0][1] = deg2rad(-motor_1)
        #state.joint_angles[0][3] = deg2rad(-motor_1)
        state.joint_angles[2][0] = deg2rad(motor_2)
        state.joint_angles[2][1] = deg2rad(motor_2)
        state.joint_angles[2][2] = deg2rad(motor_2)
        state.joint_angles[2][3] = deg2rad(motor_2)
        
        
        joint_checker.check_limit (state.joint_angles)

        # Update the pwm widths going to the servos
        hardware_interface.set_actuator_postions(state.joint_angles)

        #Update the previous joint angle state
        joint_checker.update(state.joint_angles)

        motor_2 += 2
        print(motor_2)
        if (motor_2 >= 180 and motor_2 <= 182): 
            motor_1 += 30
            state.joint_angles[1][0] = deg2rad(motor_1)
            state.joint_angles[1][1] = deg2rad(motor_1)
            state.joint_angles[1][2] = deg2rad(motor_1)
            state.joint_angles[1][3] = deg2rad(motor_1)
    

    #####################################################################################################clockwise Testing
    """
    state.joint_angles[2][0] = deg2rad(motor_2)
    motor_2 += 2
    if (motor_2 >= 180 and motor_2 <= 182): 
        motor_1 += 30
    
    #print("angle1", angle1)
    #print("angle2", angle2)
    #angle1 -= 1 
    #angle2 -= 1
    #temp -= 2
    #angle1 -= 2
    #angle1 += 1 

    #########################################MOTOR 2 TEST
    """
    #motor_2 += 1
    #if motor_2 == 55 :
    #    motor_3 += 10    
    """
    #########################################MOTOR 3 TEST 
    ###### - rotation 
    #motor_3 += 1 
    #if (motor_3 == 153):
    """
main()
