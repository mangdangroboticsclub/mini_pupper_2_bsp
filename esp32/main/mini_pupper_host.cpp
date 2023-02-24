/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

static const char* TAG = "HOST";


#include "mini_pupper_host.h"
#include "mini_pupper_servos.h"
#include "mini_pupper_imu.h"
#include "mini_pupper_power.h"
#include "mini_pupper_tasks.h"
#include "mini_pupper_stats.h"


#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define HOST_SERVER_TXD 17
#define HOST_SERVER_RXD 18
#define HOST_SERVER_RTS (UART_PIN_NO_CHANGE)
#define HOST_SERVER_CTS (UART_PIN_NO_CHANGE)

void HOST_TASK(void * parameters);

HOST host;

HOST::HOST() : 
_uart_port_num(2),
_is_service_enabled(false),
_task_handle(NULL),
_uart_queue(NULL),
f_monitor(_protocol_handler.f_monitor)
{
    // set UART port
    uart_config_t uart_config;
    uart_config.baud_rate = 3000000;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(_uart_port_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(_uart_port_num, HOST_SERVER_TXD, HOST_SERVER_RXD, HOST_SERVER_RTS, HOST_SERVER_CTS));
    ESP_ERROR_CHECK(uart_driver_install(_uart_port_num, 1024, 1024, 40, &_uart_queue, 0));
}

static size_t const stack_size = 10000;
static StackType_t stack[stack_size] {0};
static StaticTask_t task_buffer;

void HOST::start()
{
    _task_handle = xTaskCreateStaticPinnedToCore(
        HOST_TASK,   
        "HOST INTERFACE SERVICE",
        stack_size,         
        (void*)this,        
        HOST_PRIORITY,     
        stack,
        &task_buffer,
        HOST_CORE
    );
}

void HOST::enable_service(bool enable)
{
    _is_service_enabled = enable;
}

void HOST_TASK(void * parameters)
{
    HOST * host = reinterpret_cast<HOST*>(parameters);
    uart_event_t event;
    u8 rx_buffer[1024] {0};
    protocol_interpreter_handler & protocol_handler {host->_protocol_handler};
    for(;;)
    {
        // Waiting for UART event.
        if(xQueueReceive(host->_uart_queue,(void*)&event,(TickType_t)portMAX_DELAY))
        {
            switch (event.type)
            {

            // Event of UART receving data
            // We'd better handler data event fast, there would be much more data events than
            // other types of events. If we take too much time on data event, the queue might be full.                
            case UART_DATA:
                {
                    // log
                    ESP_LOGD(TAG, "RX uart event size: %d", event.size);

                    // read a frame from host
                    int const read_length {uart_read_bytes(host->_uart_port_num,rx_buffer,event.size,portMAX_DELAY)};

                    // decode received data
                    bool have_to_reply {false};
                    for(size_t index=0; index<read_length; ++index)
                    {
                        bool const payload = protocol_interpreter(rx_buffer[index],protocol_handler);
                        if(payload)
                        {
                            // waitinf for a INST_CONTROL frame
                            if(protocol_handler.payload_buffer[0]==INST_CONTROL && protocol_handler.payload_length == sizeof(parameters_control_instruction_format)+2)
                            {
                                // decode parameters
                                parameters_control_instruction_format parameters;
                                memcpy(&parameters,&protocol_handler.payload_buffer[1],sizeof(parameters_control_instruction_format));

                                // log
                                ESP_LOGD(TAG, "Goal Position: %d %d %d %d %d %d %d %d %d %d %d %d",
                                    parameters.goal_position[0],parameters.goal_position[1],parameters.goal_position[2],
                                    parameters.goal_position[3],parameters.goal_position[4],parameters.goal_position[5],
                                    parameters.goal_position[6],parameters.goal_position[7],parameters.goal_position[8],
                                    parameters.goal_position[9],parameters.goal_position[10],parameters.goal_position[11]
                                );
                                ESP_LOGD(TAG, "Torque Switch: %d %d %d %d %d %d %d %d %d %d %d %d",
                                    parameters.torque_enable[0],parameters.torque_enable[1],parameters.torque_enable[2],
                                    parameters.torque_enable[3],parameters.torque_enable[4],parameters.torque_enable[5],
                                    parameters.torque_enable[6],parameters.torque_enable[7],parameters.torque_enable[8],
                                    parameters.torque_enable[9],parameters.torque_enable[10],parameters.torque_enable[11]
                                );

                                // update servo setpoint only if service is enabled
                                if(host->_is_service_enabled)
                                {
                                    servo.setTorque12Async(parameters.torque_enable);
                                    servo.setPosition12Async(parameters.goal_position);
                                }

                                // send have_to_reply
                                have_to_reply = true;

                            }
                            else
                            {
                                ESP_LOGI(TAG, "RX unexpected frame. Instr:%d. Length:%d",protocol_handler.payload_buffer[0],protocol_handler.payload_length);        
                                host->f_monitor.update(mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR, false);
                            }
                        }
                    }

                    // have to reply ?
                    if(have_to_reply)
                    {
                        // servo feedback
                        parameters_control_acknowledge_format feedback_parameters;
                        servo.getPosition12Async(feedback_parameters.present_position);
                        servo.getLoad12Async(feedback_parameters.present_load);
                        // imu feedback
                        feedback_parameters.ax = imu.ax;
                        feedback_parameters.ay = imu.ay;
                        feedback_parameters.az = imu.az;
                        feedback_parameters.gx = imu.gx;
                        feedback_parameters.gy = imu.gy;
                        feedback_parameters.gz = imu.gz;
                        // power supply feedback
                        feedback_parameters.voltage_V = POWER::get_voltage_V();
                        feedback_parameters.current_A = POWER::get_current_A();

                        // build acknowledge frame
                        static size_t const tx_payload_length {1+sizeof(parameters_control_acknowledge_format)+1};            
                        static size_t const tx_buffer_size {4+tx_payload_length};            
                        u8 tx_buffer[tx_buffer_size] {
                            0xFF,                                       // Start of Frame
                            0xFF,                                       // Start of Frame
                            0x01,                                       // ID
                            tx_payload_length,                          // Length
                            0x00,                                       // Status
                        };
                        memcpy(tx_buffer+5,&feedback_parameters,sizeof(parameters_control_acknowledge_format));

                        // compute checksum
                        tx_buffer[tx_buffer_size-1] = compute_checksum(tx_buffer);

                        // send frame to host
                        uart_write_bytes(host->_uart_port_num,tx_buffer,tx_buffer_size);

                        // Wait for packet to be sent
                        //ESP_ERROR_CHECK(uart_wait_tx_done(host->_uart_port_num, 10)); // wait timeout is 10 RTOS ticks (TickType_t)
                    }

                    // stats
                    host->p_monitor.update();

                }
                break;

            // Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(host->_uart_port_num);
                xQueueReset(host->_uart_queue);
                break;

            // Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(host->_uart_port_num);
                xQueueReset(host->_uart_queue);
                break;

            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;

            // Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;

            // Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;       

            } // Switch event

        } // xQueue

    } // for

}
