/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_protocol_H
#define _mini_pupper_protocol_H

#include "mini_pupper_types.h"
#include <string.h>

inline u8 compute_checksum(u8 const buffer[])
{
    size_t const frame_size { (size_t)(buffer[3]+4) };
    u8 chk_sum = 0;
    for(size_t index=2; index<(frame_size-1); ++index)
    {
        chk_sum += buffer[index];
    }
    return ~chk_sum;
}

inline bool checksum(u8 const buffer[], u8 & expected_checksum)
{
    size_t const frame_size { (size_t)(buffer[3]+4) };
    u8 const received_checksum {buffer[frame_size-1]};
    expected_checksum = compute_checksum(buffer);
    return received_checksum==expected_checksum;
}

#endif //_mini_pupper_protocol_H
