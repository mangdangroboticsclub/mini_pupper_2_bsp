/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_stats_H
#define _mini_pupper_stats_H

#include "esp_timer.h"

#include <algorithm>
#include <cmath>

namespace mini_pupper
{


    // Monitor a periodic process
    //  - count
    //  - frequency (min,mean,max,variance)
    struct periodic_process_monitor
    {
        periodic_process_monitor(float alpha = 0.0002f) :
        _alpha(alpha)
        {

        }

        void update()
        {
            // compute delta us
            int64_t const current_time_us { esp_timer_get_time() };
            int64_t const delta_time_us = current_time_us-_last_time_us;
            _last_time_us = current_time_us;

            // when ready (3sec), compute stats
            float const delta_time_s = 0.000001f * (float)delta_time_us;
            float const frequency = 1.0f/delta_time_s;
            if(counter++<1000)
            {
                frequency_mean    = (1.0f-0.005f)*frequency_mean + 0.005f*frequency;
                frequency_var     = (1.0f-0.005f)*frequency_var  + 0.005f*powf(frequency-frequency_mean,2.0f);
                frequency_min = frequency_mean;
                frequency_max = frequency_mean;
            }
            else
            {
                frequency_mean    = (1.0f-_alpha)*frequency_mean + _alpha*frequency;
                frequency_var     = (1.0f-_alpha*0.01f)*frequency_var  + _alpha*0.01f*powf(frequency-frequency_mean,2.0f);
                frequency_min = std::min(frequency_mean,frequency_min); 
                frequency_max = std::max(frequency_mean,frequency_max);
                //frequency_min = (1.0f-0.99f)*_alpha*frequency_min+0.01f*_alpha*frequency_mean;
                //frequency_max = (1.0f-0.99f)*_alpha*frequency_min+0.01f*_alpha*frequency_mean;                
            }
        }


        uint64_t counter        {0};
        float frequency_var     {0.0f};
        float frequency_min     {0.0f};
        float frequency_mean    {0.0f};
        float frequency_max     {0.0f};

        private:

            float _alpha {0.0002f};
            int64_t _last_time_us {0};
    };


    // Monitor a frame error rate communication
    //  - transmission count
    //  - checksum error count
    //  - syntax error count
    //  - time-out count
    struct frame_error_rate_monitor
    {
        enum error_type
        {
            ALL = 0,
            CHECKSUM_ERROR,
            SYNTAX_ERROR,
            TIME_OUT_ERROR,
            TRUNCATED_ERROR,
            ERROR_COUT
        };

        static constexpr error_type all[] {CHECKSUM_ERROR,SYNTAX_ERROR,TIME_OUT_ERROR};

        void update(error_type error = ALL, bool increment_ALL = true)
        {
            if(increment_ALL)
            {
                ++counter[ALL];
            }   
            if(error!=ALL)
            {
                ++counter[error];
            }
        }

        void compute_rates()
        {
            for(auto & e : all)
                rate[e] = static_cast<double>(counter[e])/static_cast<double>(counter[ALL]);
        }

        uint64_t counter[ERROR_COUT]        {0};
        double   rate[ERROR_COUT]           {0.0};
    };

};

#endif //_mini_pupper_stats_H
