/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_host_H
#define _mini_pupper_host_H

#include "mini_pupper_types.h"
#include "mini_pupper_stats.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"

/* IMPORTANT : Mini Pupper Host API requires to setup FreeRTOS frequency at 1000Hz.
 *             Use IDF ESP32 : MENUCONFIG > COMPONENTS > FREERTOS > KERNEL > 1000Hz
 *
 */

#include "mini_pupper_host_base.h"
#include "mini_pupper_protocol.h"

void HOST_TASK(void * parameters);

struct HOST
{
    HOST();

    void start();

    void enable_service(bool enable = true);

private:

    int _uart_port_num {2};
    bool _is_service_enabled {false};
    
    protocol_interpreter_handler _protocol_handler;

    // background host serial bus service
    TaskHandle_t _task_handle {NULL};    
    QueueHandle_t _uart_queue {NULL};    
    friend void HOST_TASK(void * parameters);

public:

    // public stats
    mini_pupper::periodic_process_monitor p_monitor;
    mini_pupper::frame_error_rate_monitor & f_monitor;

};

extern HOST host; 

#endif //_mini_pupper_host_H
