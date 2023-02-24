/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_power_H
#define _mini_pupper_power_H

#include "mini_pupper_types.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/queue.h"

#include "esp_adc/adc_continuous.h"


namespace POWER
{
    void start();

    float get_voltage_V();
    float get_current_A();    

};

#endif //_mini_pupper_power_H
