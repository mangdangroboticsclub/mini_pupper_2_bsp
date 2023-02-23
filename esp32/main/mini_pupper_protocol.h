/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_protocol_H
#define _mini_pupper_protocol_H

#include "mini_pupper_types.h"
#include "mini_pupper_stats.h"
#include <string.h>

///#include "esp_log.h"

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

enum protocol_interpreter_state
{
    HEADER1,
    HEADER2,
    ID,
    LENGTH,
    PAYLOAD,
    CHECKSUM
};

struct protocol_interpreter_handler
{
    protocol_interpreter_state state{HEADER1};
    u8 payload_length {0};
    u8 payload_byte_cout {0};
    u8 checksum {0};
    static u8 const MAX_PAYLOAD_LENGTH {128};
    u8 payload_buffer[MAX_PAYLOAD_LENGTH] {0};
    mini_pupper::frame_error_rate_monitor f_monitor;
};

inline bool protocol_interpreter(u8 input_byte, protocol_interpreter_handler & handler)
{
    switch(handler.state)
    {
    default:
    case HEADER1: // wait for a new frame starting with 0xFF
        {
            if(input_byte==0xFF) handler.state = HEADER2;
        }
        break;
    case HEADER2: // waiting for a second 0xFF
        {
            if(input_byte==0xFF) handler.state = ID;
            else                 handler.state = HEADER1;   
        }
        break;
    case ID: // waiting for an ID (=0x01)
        {
            if(input_byte==0x01)      handler.state = LENGTH;
            else if(input_byte==0xFF) handler.state = ID;
            else
            {
                handler.state = HEADER1;   
                handler.f_monitor.update(mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR);
            }
            handler.checksum = input_byte;
        }
        break;
    case LENGTH: // waiting for a length
        {
            handler.payload_length = input_byte;
            handler.checksum += input_byte;
            handler.payload_byte_cout = 0;
            if(handler.payload_length>0 && handler.payload_length<handler.MAX_PAYLOAD_LENGTH) handler.state = PAYLOAD; // reject empty payload, reject too large payload
            else
            {
                handler.state = HEADER1;   
                handler.f_monitor.update(mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR);
            }                
        }
        break;
    case PAYLOAD: // waiting for a length
        {
            handler.payload_buffer[handler.payload_byte_cout++]=input_byte;
            handler.checksum += input_byte;
            if(handler.payload_byte_cout==handler.payload_length-1) handler.state = CHECKSUM;
        }
        break;
    case CHECKSUM: // process checksum
        {
            handler.state = HEADER1;
            // checksum
            if(input_byte==(u8)(~handler.checksum))
            {
                handler.f_monitor.update();
                return true; // payload ready to decode
            }
            else
            {
                handler.f_monitor.update(mini_pupper::frame_error_rate_monitor::CHECKSUM_ERROR);
            }
            
            ///ESP_LOGI("PROTOCOL", "RX frame checksum FAIL! length:%d rcv_chk:%d exp_chk:%d",handler.payload_length,input_byte,(u8)(~handler.checksum));
        }
        break;
    }
    return false; // no valid payload found
}

#endif //_mini_pupper_protocol_H
