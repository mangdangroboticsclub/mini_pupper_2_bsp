/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#include "mini_pupper_servos.h"
#include "mini_pupper_math.h"
#include "mini_pupper_tasks.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "hal/gpio_hal.h"
#include "esp_log.h"
#include <string.h>

// reference :
//https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html

static char const * TAG {"SERVOS"};

SERVO servo;

SERVO::SERVO()
{
    // setup enable pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;//disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;//set as output mode
    io_conf.pin_bit_mask = (1ULL<<8);//bit mask of the pins that you want to set,e.g.GPIO18
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;//disable pull-down mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;//disable pull-up mode
    ESP_ERROR_CHECK(gpio_config(&io_conf));//configure GPIO with the given settings

    // power off servo system
    enable_power(false);

    // set UART port
    uart_config_t uart_config;
    uart_config.baud_rate = 500000;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 122;
#if SOC_UART_SUPPORT_REF_TICK
    uart_config.source_clk = UART_SCLK_REF_TICK;
#elif SOC_UART_SUPPORT_XTAL_CLK
    uart_config.source_clk = UART_SCLK_XTAL;
#endif
    uart_port_num = UART_NUM_1;
    ESP_ERROR_CHECK(uart_driver_install(uart_port_num, 1024, 1024, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_port_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_port_num, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static size_t const stack_size = 10000;
static StackType_t stack[stack_size] {0};
static StaticTask_t task_buffer;

void SERVO::start()
{
    /*** ASYNC API service ***/
    _task_handle = xTaskCreateStaticPinnedToCore(
        SERVO_TASK,   
        "SERVO BUS SERVICE",
        stack_size,         
        (void*)this,        
        SERVO_PRIORITY,     
        stack,
        &task_buffer,
        SERVO_CORE
    );
}

bool SERVO::is_power_enabled() const
{
    return _is_power_enabled;
}

void SERVO::enable_power(bool enable)
{
    if(enable)
    {
        gpio_set_level(GPIO_NUM_8, 1);
        _is_power_enabled = true;
        _is_service_enabled = false;
    }
    else
    {
        gpio_set_level(GPIO_NUM_8, 0);
        _is_power_enabled = false;
        _is_service_enabled = false;
    }
}

int SERVO::ping(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // abort command when broadcasting
    if(ID==0XFE) return SERVO_STATUS_FAIL;

    // send ping instruction
    write_frame(ID,INST_PING,nullptr,0);

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::reset(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // abort command when broadcasting
    if(ID==0XFE) return SERVO_STATUS_FAIL;

    // send ping instruction
    write_frame(ID,INST_RECOVERY,nullptr,0);

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::enable_torque(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // write instruction
    write_register_byte(ID, SERVO_TORQUE_ENABLE, 1);

    // if broadcast, do not wait for reply
    if(ID==0XFE) return SERVO_STATUS_OK;    

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::disable_torque(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send write frame
    write_register_byte(ID, SERVO_TORQUE_ENABLE, 0);
    
    // if broadcast, do not wait for reply
    if(ID==0XFE) return SERVO_STATUS_OK;    

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::is_torque_enable(u8 ID, u8 & enable)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    int const status = read_register_byte(ID, SERVO_TORQUE_ENABLE,enable);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK;    
}

int SERVO::set_goal_position(u8 ID, u16 position)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // abort command when broadcasting
    if(ID==0XFE) return SERVO_STATUS_FAIL;

    // send write instruction
    write_register_word(ID, SERVO_GOAL_POSITION_L, position);

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::get_goal_speed(u8 ID, u16 & speed)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    int const status = read_register_word(ID, SERVO_GOAL_SPEED_L,speed);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK;
}

int SERVO::set_goal_speed(u8 ID, u16 speed)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // abort command when broadcasting
    if(ID==0XFE) return SERVO_STATUS_FAIL;

    // send write instruction
    write_register_word(ID, SERVO_GOAL_SPEED_L, speed);

    // wait for reply
    return check_reply_frame_no_parameter(ID);
}

int SERVO::get_position(u8 ID, u16 & position)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    int const status = read_register_word(ID, SERVO_PRESENT_POSITION_L,position);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK;
}

int SERVO::get_speed(u8 ID, s16 & speed)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u16 present_value {0};
    int const status = read_register_word(ID, SERVO_PRESENT_SPEED_L,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // make signed
    speed = (s16)present_value;
    if(speed&(1<<15))
        speed = -(speed&~(1<<15));

    return SERVO_STATUS_OK;
}

int SERVO::get_load(u8 ID, s16 & load)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u16 present_value {0};
    int const status = read_register_word(ID, SERVO_PRESENT_LOAD_L,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // make signed
    load = (s16)present_value;
    if(load&(1<<10))
        load = -(load&~(1<<10));

    return SERVO_STATUS_OK;
}

int SERVO::get_voltage(u8 ID, u8 & voltage)
{
#ifdef SERVO_USE_SCS_GENERIC

    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u8 present_value {0};
    int const status = read_register_byte(ID, SERVO_PRESENT_VOLTAGE,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    voltage = present_value;

    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_GENERIC
#ifdef SERVO_USE_SCS_0009

    voltage = 0;
    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_0009
}

int SERVO::get_temperature(u8 ID, u8 & temperature)
{
#ifdef SERVO_USE_SCS_GENERIC

    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u8 present_value {0};
    int const status = read_register_byte(ID, SERVO_PRESENT_TEMPERATURE,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    temperature = present_value;

    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_GENERIC
#ifdef SERVO_USE_SCS_0009

    temperature = 0;
    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_0009    
}


int SERVO::get_move(u8 ID, u8 & move)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u8 present_value {0};
    int const status = read_register_byte(ID, SERVO_MOVING,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    move = present_value;

    return SERVO_STATUS_OK;
}

int SERVO::get_current(u8 ID, s16 & current)
{
#ifdef SERVO_USE_SCS_GENERIC

    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // send read instruction
    u16 present_value {0};
    int const status = read_register_word(ID, SERVO_PRESENT_CURRENT_L,present_value);
    ESP_LOGE(TAG, "Reply error [%d]",status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // make signed
    current = (s16)present_value;
    if(current&(1<<15))
        current = -(current&~(1<<15));

    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_GENERIC
#ifdef SERVO_USE_SCS_0009

    current = 0;
    return SERVO_STATUS_OK;

#endif //SERVO_USE_SCS_0009       
}

int SERVO::unlock_eeprom(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // write instruction
    write_register_byte(ID, SERVO_LOCK, 0);

    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    int const status = reply_frame(reply_id,reply_state,nullptr,0,100);
    if(status)
        ESP_LOGI(TAG, "Reply error [%d,%d,%d]",status,reply_id,reply_state);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_id!=ID || reply_state!=0) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK;  
}

int SERVO::lock_eeprom(u8 ID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    // write instruction
    write_register_byte(ID, SERVO_LOCK, 1);

    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    int const status = reply_frame(reply_id,reply_state,nullptr,0,100);
    if(status)
        ESP_LOGI(TAG, "Reply error [%d,%d,%d]",status,reply_id,reply_state);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_id!=ID || reply_state!=0) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK; 
}

void SERVO::set_position_all(u16 const servoPositions[])
{
    // suspend sync service
    enable_service(false);

    static size_t const L {2};                      // Length of data sent to each servo
    static size_t const N {12};                     // Servo Number
    static size_t const Length {(L+1)*N+4};         // Length field value
    static size_t const buffer_size {2+1+1+Length}; // 0xFF 0xFF ID LENGTH (INSTR PARAM... CHK)
    // prepare frame header and parameters (fixed)
    static u8 buffer[buffer_size] {
        0xFF,                                       // Start of Frame
        0xFF,                                       // Start of Frame
        0xFE,                                       // ID
        Length,                                     // Length
        INST_SYNC_WRITE,                            // Instruction
        SERVO_GOAL_POSITION_L,                      // Parameter 1 : Register address
        L                                           // Parameter 2 : L
    };
    // build frame payload
    size_t index {7};
    static u8 const servoIDs[] {1,2,3,4,5,6,7,8,9,10,11,12};
    for(size_t servo_index=0; servo_index<N; ++servo_index) {
        buffer[index++] = servoIDs[servo_index];            // Parameter 3 = Servo Number
        buffer[index++] = (servoPositions[servo_index]>>8); // Write the first data of the first servo
        buffer[index++] = (servoPositions[servo_index]&0xff);
    }
    // compute checksum
    u8 chk_sum {0};
    for(size_t chk_index=2; chk_index<(buffer_size-1); ++chk_index) {
        chk_sum += buffer[chk_index];
    }
    buffer[index++] = ~chk_sum;
    // send frame to uart
    uart_write_bytes(uart_port_num, buffer, buffer_size);
}

int SERVO::setID(u8 servoID, u8 newID)
{
    // abort if servo not powered on
    if(!_is_power_enabled) return SERVO_STATUS_FAIL;

    // suspend sync service
    enable_service(false);

    unlock_eeprom(servoID);

    // change ID register
    write_register_byte(servoID, SERVO_ID, newID);

    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    int const status = reply_frame(reply_id,reply_state,nullptr,0,100);
    if(status)
        ESP_LOGI(TAG, "Reply error [%d,%d,%d]",status,reply_id,reply_state);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_state!=0) return SERVO_STATUS_FAIL;

    lock_eeprom(newID);

    return SERVO_STATUS_OK;
}


void SERVO::setTorqueAsync(u8 servoID, u8 servoTorque)
{
    if(0<servoID && servoID<=12)
        state[servoID-1].torque_enable=servoTorque;
    
    // (re)start sync service
    enable_service();
}

void SERVO::setTorque12Async(u8 const servoTorques[])
{
    for(size_t index=0;index<12;++index)
        state[index].torque_enable = servoTorques[index];
    
    // (re)start sync service
    enable_service();
}

void SERVO::setPositionAsync(u8 servoID, u16 servoPosition)
{
    if(0<servoID && servoID<=12)
        state[servoID-1].goal_position=servoPosition; // TODO : take in account calibration_offset and constrain from 0 to 1023 !
    
    // (re)start sync service
    enable_service();
}

void SERVO::setPosition12Async(u16 const servoPositions[])
{
    for(size_t index=0;index<12;++index)
        state[index].goal_position = servoPositions[index]; // TODO : take in account calibration_offset and constrain from 0 to 1023 !
    
    // (re)start sync service
    enable_service();
}

void SERVO::getGoalPosition12Async(u16 servoPositions[])
{
    // (re)start sync service
    enable_service();

    for(size_t index=0;index<12;++index)
        servoPositions[index] = state[index].goal_position;
}

void SERVO::getPosition12Async(u16 servoPositions[])
{
    // (re)start sync service
    enable_service();

    for(size_t index=0;index<12;++index)
        servoPositions[index] = state[index].present_position;
}

void SERVO::getSpeed12Async(s16 servoSpeeds[])
{
    // (re)start sync service
    enable_service();

    for(size_t index=0;index<12;++index)
        servoSpeeds[index] = state[index].present_speed;
}

void SERVO::getLoad12Async(s16 servoLoads[])
{
    // (re)start sync service
    enable_service();

    for(size_t index=0;index<12;++index)
        servoLoads[index] = state[index].present_load;
}

u16  SERVO::getPositionAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_position;
    else
        return 0;
}

s16  SERVO::getSpeedAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_speed;
    else
        return 0;
}

s16  SERVO::getLoadAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_load;
    else
        return 0;
}

u8  SERVO::getVoltageAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_load;
    else
        return 0;
}

u8  SERVO::getTemperatureAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_temperature;
    else
        return 0;
}

u8  SERVO::getMoveAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_move;
    else
        return 0;
}

s16 SERVO::getCurrentAsync(u8 servoID)
{
    // (re)start sync service
    enable_service();

    if(0<servoID && servoID<=12)
        return state[servoID-1].present_current;
    else
        return 0;
}

void SERVO::enable_service(bool enable)
{
    // cannot start is power off
    if(!_is_power_enabled) return;

    // nothing to do
    if(_is_service_enabled==enable) return;

    // (re)start sync service
    if(enable)
    {
        // (re)start sync task and wait a moment to synchronise local feedback data base
        _is_service_enabled = true; 
        vTaskDelay(20 / portTICK_PERIOD_MS);   
    }
    // suspend sync service
    else
    {
        // suspend sync task and wait a moment
        _is_service_enabled = false;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    // flush RX FIFO
    uart_flush(uart_port_num);    
}

void SERVO::sync_all_goal_position()
{
    static size_t const buffer_size {64};               // Buffer size
    static u8 buffer[buffer_size] {0xFF,0xFF,0xFE};     // Fixed header (boradcast)
    /*
     * First sync write frame : torque switch disable
     *
     *  A servo receiving a goal position automatically switches to torque enable.
     *  So we send only torque disable to servo to be disabled.
     *
     */
    {
        // prepare sync write frame to servo
        static size_t const L {1};  // Length of data sent to each servo : torque switch register is one-byte
        // prepare pyaload : instruction and parameters
        buffer[4] = INST_SYNC_WRITE;        // Instruction
        buffer[5] = SERVO_TORQUE_ENABLE;    // Parameter 1 : Register address
        buffer[6] = L;                      // Parameter 2 : L
        // build frame payload
        size_t index {7};
        size_t n {0}; // effective servo count
        for(auto & servo : state)
        {
            if(servo.torque_enable==0)
            {
                ++n;
                buffer[index++] = servo.ID;                    // Parameter 3 = Servo Number
                buffer[index++] = servo.torque_enable;         // Write the data
            }
        }
        // compute payload length
        buffer[3] = (L+1)*n+4;
        // compute checksum
        u8 chk_sum {0};
        for(size_t chk_index=2; chk_index<index; ++chk_index) {
            chk_sum += buffer[chk_index];
        }
        buffer[index++] = ~chk_sum;
        // send frame to all servo    
        uart_write_bytes(uart_port_num,buffer,index);
    }
    /*
     * Second sync write frame : goal position
     *
     */
    {
        // prepare sync write frame to servo
        static size_t const L {2};  // Length of data sent to each servo : goal position register is one-word
        // prepare pyaload : instruction and parameters
        buffer[4] = INST_SYNC_WRITE;        // Instruction
        buffer[5] = SERVO_GOAL_POSITION_L;  // Parameter 1 : Register address
        buffer[6] = L;                      // Parameter 2 : L
        // build frame payload
        size_t index {7};
        size_t n {0}; // effective servo count
        for(auto & servo : state)
        {
            if(servo.torque_enable==1)
            {
                ++n;
                buffer[index++] = servo.ID;                    // Parameter 3 = Servo Number
                // apply calibration
                u16 const raw_position {calibrated_to_raw_position(servo.goal_position,servo.calibration_offset)};
                buffer[index++] = (raw_position>>8);    
                buffer[index++] = (raw_position&0xff);                
            }
        }
        // compute payload length
        buffer[3] = (L+1)*n+4;
        // compute checksum
        u8 chk_sum {0};
        for(size_t chk_index=2; chk_index<index; ++chk_index) {
            chk_sum += buffer[chk_index];
        }
        buffer[index++] = ~chk_sum;
        // send frame to all servo    
        uart_write_bytes(uart_port_num,buffer,index);
    }
}

void SERVO::cmd_feedback_one_servo(SERVO_STATE & servoState)
{
    // prepare read frame to one servo
    static size_t const buffer_size {8};            
    u8 buffer[buffer_size] {
        0xFF,                                       // Start of Frame
        0xFF,                                       // Start of Frame
        servoState.ID,                              // ID
        0x04,                                       // Length of read instruction
        INST_READ,                                  // Read instruction
        SERVO_PRESENT_POSITION_L,                   // Parameter 1 : Register address
        0x06,                                       // Parameter 2 : 6 bytes (position, speed, load)
        0x00                                        // checksum
    };
    // compute checksum
    u8 chk_sum {0};
    for(size_t chk_index=2; chk_index<(buffer_size-1); ++chk_index) {
        chk_sum += buffer[chk_index];
    }
    buffer[buffer_size-1] = ~chk_sum;
    // send frame to servo
    uart_write_bytes(uart_port_num,buffer,buffer_size);
    // flush RX FIFO
    uart_flush(uart_port_num);
}

void SERVO::ack_feedback_one_servo(SERVO_STATE & servoState)
{
    // a buffer to process read ack from one servo
    static size_t const buffer_size {12};     
    u8 buffer[buffer_size] {0};
    // copy RX fifo into local buffer
    int const read_length = uart_read_bytes(uart_port_num,buffer,buffer_size,1);
    // check expected frame size
    if(read_length==buffer_size)
    {
        // check frame header, servo id, instruction, length and working condition returned by servo
        if( buffer[0] == 0xFF &&             // Start of Frame
            buffer[1] == 0xFF &&             // Start of Frame
            buffer[2] == servoState.ID &&    // ID
            buffer[3] == 0x08 &&             // Length of read reply
            buffer[4] == 0x00                // Length of working condition
        )
        {
            // compute checksum
            u8 chk_sum {0};
            for(size_t chk_index=2; chk_index<(buffer_size-1); ++chk_index) {
                chk_sum += buffer[chk_index];
            }   
            // check checksum
            if(buffer[buffer_size-1]==(u8)(~chk_sum))
            {
                // decode parameters and update feedback local data base for this servo
                u16 const raw_position = (u16)(buffer[5])<<8 | buffer[6];
                u16 const speed  = (u16)(buffer[7])<<8 | buffer[8];
                u16 const load   = (u16)(buffer[9])<<8 | buffer[10];

                // apply calibration
                servoState.present_position = raw_to_calibrated_position(raw_position,servoState.calibration_offset);

                // .. to signed values
                servoState.present_speed = (s16)speed;
                if(servoState.present_speed&(1<<15))
                    servoState.present_speed = -(servoState.present_speed&~(1<<15));

                servoState.present_load =  (s16)load;
                if(servoState.present_load&(1<<10))
                    servoState.present_load = -(servoState.present_load&~(1<<10));

                // stats
                f_monitor.update(); // OK
            }
            else
            {
                // stats
                f_monitor.update(mini_pupper::frame_error_rate_monitor::CHECKSUM_ERROR);
            }
        }
        else
        {
            // stats
            f_monitor.update(mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR);
        }
    }
    else if(read_length>0)
    {
        // stats
        f_monitor.update(mini_pupper::frame_error_rate_monitor::TRUNCATED_ERROR);
    }
    else if(read_length==0)
    {
        // stats
        f_monitor.update(mini_pupper::frame_error_rate_monitor::TIME_OUT_ERROR);
    }    
    // flush RX FIFO
    uart_flush(uart_port_num); 
}

void SERVO_TASK(void * parameters)
{
    SERVO * servo = reinterpret_cast<SERVO*>(parameters);
    u8 servoID {0};
    for(;;)
    {
        if(servo->_is_power_enabled && servo->_is_service_enabled)
        {
            // process read ack from one servo
            servo->ack_feedback_one_servo(servo->state[servoID]);
            // sync write setpoint to all servo
            servo->sync_all_goal_position();
            // basic round robin algorithm for feedback
            servoID = (servoID+1)%12;
            // read one servo feedback
            servo->cmd_feedback_one_servo(servo->state[servoID]);

            // stats
            servo->p_monitor.update();

        }
        // delay 1ms
        // - about 1KHz refresh frequency for sync write servo setpoints
        // - about 80Hz refresh frequency for read/ack servo feedbacks
        vTaskDelay(2 / portTICK_PERIOD_MS);

    }
}

int SERVO::write_frame(u8 ID, u8 instruction, u8 const * parameters, size_t parameter_length)
{
    // prepare frame to one servo
    size_t const length {parameter_length+2};       // Length : instruction + params + checksum
    size_t const buffer_size {2+1+1+length};        // 0xFF 0xFF ID LENGTH (INSTR PARAM... CHK)    
    u8 buffer[buffer_size] {
        0xFF,                                       // Start of Frame
        0xFF,                                       // Start of Frame
        ID,                                         // ID
        (u8)length,                                 // Length of payload
        instruction                                 // Instruction
    };
    // fill payload
    if(parameters)
    {
        for(size_t index=0; index<parameter_length; ++index)
        {
            buffer[index+5]=parameters[index];
        }
    }
    else
    {
        for(size_t index=0; index<parameter_length; ++index)
        {
            buffer[index+5]=0;
        }        
    }
    // compute checksum and fill payload
    u8 chk_sum {0};
    for(size_t chk_index=2; chk_index<(buffer_size-1); ++chk_index)
    {
        chk_sum += buffer[chk_index];
    }
    buffer[buffer_size-1] = ~chk_sum;
    // flush RX FIFO
    uart_flush(uart_port_num);  
    // send frame to servo
    uart_write_bytes(uart_port_num,buffer,buffer_size);

    return SERVO_STATUS_OK;
}

int SERVO::reply_frame(u8 & ID, u8 & state, u8 * parameters, size_t parameter_length, TickType_t timeout)
{
    // a buffer to process reply packet from one servo
    // prepare frame to one servo
    size_t const length {parameter_length+2};       // Length : state + params + checksum
    size_t const buffer_size {2+1+1+length};        // 0xFF 0xFF ID LENGTH (STATE PARAM... CHK)    
    u8 buffer[buffer_size] {0};
    // copy RX fifo into local buffer
    int const read_length = uart_read_bytes(uart_port_num,buffer,buffer_size,timeout);
    // flush RX FIFO
    uart_flush(uart_port_num);    
    // check expected frame size
    ///printf("   buffer_size:%d read_length:%d.",buffer_size,read_length);
    if(read_length!=buffer_size) return SERVO_STATUS_FAIL;
    // check frame header
    if(buffer[0]!=0xFF || buffer[1]!=0xFF) return SERVO_STATUS_FAIL;
    // check length
    if(buffer[3]!=length) return SERVO_STATUS_FAIL;
    // compute checksum
    u8 chk_sum {0};
    for(size_t chk_index=2; chk_index<(buffer_size-1); ++chk_index)
    {
        chk_sum += buffer[chk_index];
    }   
    // check checksum
    if(buffer[buffer_size-1]!=(u8)(~chk_sum)) return SERVO_STATUS_FAIL;
    // extract servo ID
    ID = buffer[2];
    // extract state
    state = buffer[4];
    // extract parameters
    if(parameters)
    {
        for(size_t index=0; index<parameter_length; ++index)
        {
            parameters[index]=buffer[index+5];
        }
    }
    return SERVO_STATUS_OK;   
}

int SERVO::write_register_byte(u8 id, u8 reg, u8 value)
{
    u8 const buffer[2] {reg,value};
    write_frame(id,INST_WRITE,buffer,2);

    return SERVO_STATUS_OK;
}

int SERVO::write_register_word(u8 id, u8 reg, u16 value)
{
    u8 const buffer[3] {
        reg,
        (u8)(value>>8),
        (u8)(value&0xff)
    };
    write_frame(id,INST_WRITE,buffer,3);

    return SERVO_STATUS_OK;
}

int SERVO::read_register_byte(u8 id, u8 reg, u8 & value)
{
    // abort command when broadcasting
    if(id==0XFE) return SERVO_STATUS_FAIL;

    // send read instruction
    u8 const buffer[2] {reg,1};
    write_frame(id,INST_READ,buffer,2);

    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    u8 data[1] {0};
    int const status = reply_frame(reply_id,reply_state,data,1);
    ///printf(" reply_id:%d reply_state:%d status:%d.",reply_id,reply_state,status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_id!=id || reply_state!=0) return SERVO_STATUS_FAIL;

    // make byte
    value = data[0];

    return SERVO_STATUS_OK;
}

int SERVO::read_register_word(u8 id, u8 reg, u16 & value)
{
    // abort command when broadcasting
    if(id==0XFE) return SERVO_STATUS_FAIL;

    // send read instruction
    u8 const buffer[2] {reg,2};
    write_frame(id,INST_READ,buffer,2);

    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    u8 data[2] {0};
    int const status = reply_frame(reply_id,reply_state,data,2);
    ///printf(" reply_id:%d reply_state:%d status:%d.",reply_id,reply_state,status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_id!=id || reply_state!=0) return SERVO_STATUS_FAIL;

    // make word
    value = (u16)(data[0])<<8 | data[1];

    return SERVO_STATUS_OK;
}

int SERVO::check_reply_frame_no_parameter(u8 & ID)
{
    // wait for reply
    u8 reply_id {0};
    u8 reply_state {0};
    int const status = reply_frame(reply_id,reply_state,nullptr,0);
    ///printf(" reply_id:%d reply_state:%d status:%d.",reply_id,reply_state,status);

    // check reply
    if(status!=SERVO_STATUS_OK) return SERVO_STATUS_FAIL;

    // check reply
    if(reply_id!=ID || reply_state!=0) return SERVO_STATUS_FAIL;

    return SERVO_STATUS_OK;    
}

void SERVO::setCalibration(s16 const offset[])
{
    for(size_t index=0;index<12;++index)
        state[index].calibration_offset = offset[index];
}

void SERVO::resetCalibration()
{
    for(size_t index=0;index<12;++index)
        state[index].calibration_offset = 0;
}

u16 SERVO::raw_to_calibrated_position(u16 raw_position, s16 calibration_offset) const
{
    s16 new_position {static_cast<s16>(raw_position)};
    return (u16)std::clamp(new_position+calibration_offset,0,1023);
}

u16 SERVO::calibrated_to_raw_position(u16 calibrated_position, s16 calibration_offset) const
{
    s16 new_position {static_cast<s16>(calibrated_position)};
    return (u16)std::clamp(new_position-calibration_offset,0,1023);
}
