/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_app_H
#define _mini_pupper_app_H

enum e_mini_pupper_state
{
    STATE_IDLE,
    // Mini pupper responds to HOST and CLI. 
    // IMU is working. 
    // SERVO are powered OFF. 
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback nOK, IMU feedback ok, POWER feedback ok)
    // This is the initial state

    STATE_STANDING_UP,
    // Mini pupper stand up in a few seconds.
    // IMU is working. 
    // SERVO are powered ON.
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback OK, IMU feedback ok, POWER feedback ok)

    STATE_WALKING,
    // Mini pupper can walk. 
    // IMU is working. 
    // SERVO are powered ON.
    // HOST service is enabled (torque switch and goal position setpoint are taken in account, SERVO feedback OK, IMU feedback ok, POWER feedback ok)

    STATE_SITTING_DOWN,
    // Mini pupper sit down in a few seconds.
    // IMU is working. 
    // SERVO are powered ON.
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback OK, IMU feedback ok, POWER feedback ok)

    STATE_RESTING,
    // Mini pupper responds to HOST and CLI. 
    // IMU is working. 
    // SERVO are powered OFF.
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback nOK, IMU feedback ok, POWER feedback ok)
    // This is an end state

    STATE_CALIBRATION_START,
    STATE_CALIBRATION,
    STATE_CALIBRATION_FINISH
    // Mini pupper is calibrating.
    // IMU is working. 
    // SERVO are powered ON.
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback OK, IMU feedback ok, POWER feedback ok)
    // SERVO can be moved to ZERO/REF position by hand
};

extern e_mini_pupper_state state; 

#endif //_mini_pupper_app_H
