/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_app_H
#define _mini_pupper_app_H

#include "mini_pupper_types.h"

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"
#define CALIBRATE_PATH MOUNT_PATH "/calib.txt"

static u16 const REF_ZERO_POSITION {512}; // Hard-coded REF/ZERO position for calibration (0..1023). TODO : from Flash ?

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

    STATE_RESTING
    // Mini pupper responds to HOST and CLI. 
    // IMU is working. 
    // SERVO are powered OFF.
    // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback nOK, IMU feedback ok, POWER feedback ok)
    // This is an end state
};

extern e_mini_pupper_state state; 

#endif //_mini_pupper_app_H
