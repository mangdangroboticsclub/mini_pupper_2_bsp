/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_host_base_H
#define _mini_pupper_host_base_H

#include "mini_pupper_types.h"

// host instruction code
#define INST_CONTROL 0x01   // Host sends servo position setpoints, ESP32 replies with servo feedback, attitude, ....

// frame parameters format for control instruction
struct parameters_control_instruction_format
{
    u8 torque_enable[12];
    u16 goal_position[12];
};

// frame parameters format for control acknowledge
struct parameters_control_acknowledge_format
{
    // SERVO feedback
    u16 present_position[12];
    s16 present_load[12];
    // IMU data
    float ax, ay, az;
    float gx, gy, gz;
    // POWER SUPPLY data
    float voltage_V;
    float current_A;
};


#endif //_mini_pupper_host_base_H
