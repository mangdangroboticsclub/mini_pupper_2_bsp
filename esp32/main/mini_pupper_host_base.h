/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_host_base_H
#define _mini_pupper_host_base_H

#include "mini_pupper_types.h"


/* PROTOCOL
 *
 * Frame format : 
 *  Header    (16bits) : 0xFF 0xFF
 *  Identifier (8bits) : [0x01,0xFE]
 *  Length     (8bits) : Payload length in bytes + 1
 *  Payload  (L bytes) : Variable size data payload
 *  Checksum   (8bits) : Checksum of all fields except header
 *
 *
 * Payload format :
 *  Instruction code (8bits) : [0x01..]
 *  Parameters     (N bytes) : [0..64]
 *
 *
 * CONTROL exchange :
 *
 *  1) HOST sends a CONTROL instruction.
 *     ID = 0x01
 *     Length = 26 bytes
 *     Instruction code = 0x01
 *     Parameters = 12 x position (u16) [0..1023]
 *
 *     Test frame with all servo to neutral :
 *          0xff 0xff 0x01 0x1a     Header, ID=1, Length=26
 *          0x01 0x00 0x02 0x00     Instruction=1(CONTROL), goal_position[0]=512...
 *          0x02 0x00 0x02 0x00
 *          0x02 0x00 0x02 0x00
 *          0x02 0x00 0x02 0x00
 *          0x02 0x00 0x02 0x00
 *          0x02 0x00 0x02 0x00     ...goal_position[11]=512
 *          0x02 0xcb               Checksum
 *
 *  2) ESP32 replies a CONTROL acknowledge in less than 2ms.
 *     ID = 0x01
 *     Length = 50 bytes
 *     Status code = 0x00
 *     Parameters = 
 *      * 12 x position (u16) [0..1023]
 *      * 12 x load     (s16) [-1000..+1000]
 *
 *
 */

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
    // IMU raw data
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    // POWER SUPPLY data
    float voltage_V;
    float current_A;
};


#endif //_mini_pupper_host_base_H
