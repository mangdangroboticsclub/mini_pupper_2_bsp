/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

static const char* TAG = "POWER";

#include "mini_pupper_power.h"

#include "esp_log.h"

#include <string.h>

namespace POWER
{
    static float current {0.0f};
    static float voltage {0.0f};

    static float ALPHA_VOLTAGE {0.1f};
    static float ALPHA_CURRENT {0.1f};

    static size_t const SAMPLE_COUNT {12}; // count
    static size_t const SAMPLE_FREQ {SOC_ADC_SAMPLE_FREQ_THRES_LOW}; // 611Hz

    static uint8_t adc_buffer[SAMPLE_COUNT*SOC_ADC_DIGI_DATA_BYTES_PER_CONV] {0};

    static adc_continuous_handle_t handle {NULL};
    static TaskHandle_t task_handle {NULL};
    static void POWER_TASK(void * parameters);

    static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
    {
        BaseType_t mustYield = pdFALSE;
        //Notify that ADC continuous driver has done enough number of conversions
        vTaskNotifyGiveFromISR(task_handle, &mustYield);
        return (mustYield == pdTRUE);
    }

    void start()
    {
        adc_continuous_handle_cfg_t adc_config {
            .max_store_buf_size = 1024,
            .conv_frame_size = SAMPLE_COUNT*SOC_ADC_DIGI_DATA_BYTES_PER_CONV,
        };
        ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

        size_t const CHANNEL_COUNT {2};
        adc_channel_t const channel[CHANNEL_COUNT] = {ADC_CHANNEL_0, ADC_CHANNEL_1};

        adc_digi_pattern_config_t adc_pattern[CHANNEL_COUNT];
        for (int i = 0; i < CHANNEL_COUNT; ++i)
        {
            adc_pattern[i].atten = ADC_ATTEN_DB_11;
            adc_pattern[i].channel = channel[i] & 0x7;
            adc_pattern[i].unit = ADC_UNIT_1;
            adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
        }

        adc_continuous_config_t dig_cfg {
            .pattern_num = CHANNEL_COUNT,
            .adc_pattern = adc_pattern,
            .sample_freq_hz = SAMPLE_FREQ,
            .conv_mode = ADC_CONV_SINGLE_UNIT_1,
            .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        };    
        ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

        xTaskCreate(
            POWER_TASK,         
            "POWER SERVICE",    
            10000,              
            NULL,               // Parameter passed into the task.
            0,                  // Priority
            &task_handle        // Used to pass out the created task's handle. */
        );

        adc_continuous_evt_cbs_t cbs {
            .on_conv_done = s_conv_done_cb,
            .on_pool_ovf = nullptr
        };
        ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));

        ESP_ERROR_CHECK(adc_continuous_start(handle));
    }

    void POWER_TASK(void * parameters)
    {
        for(;;)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            ESP_LOGD(TAG, "ADC conv EVENT!");
            
            uint32_t sample_count {0};
            esp_err_t const status = adc_continuous_read(handle, adc_buffer, SAMPLE_COUNT*SOC_ADC_DIGI_DATA_BYTES_PER_CONV, &sample_count, 0);
            ESP_LOGD(TAG, "ADC conv EVENT (%d) [%ld]!",status,sample_count);
            if(status==ESP_OK)
            {
                for(size_t index=0; index<sample_count; index+=SOC_ADC_DIGI_DATA_BYTES_PER_CONV)
                {
                    adc_digi_output_data_t const * sample_data = reinterpret_cast<adc_digi_output_data_t*>(&adc_buffer[index]);
                    switch(sample_data->type2.channel)
                    {
                    case ADC_CHANNEL_0: // current
                        {
                            current = ALPHA_CURRENT * sample_data->type2.data * 3.3 / 4096.0 * 10.0 / 2.5 + (1.0-ALPHA_CURRENT) * current;
                        }
                        break;
                    case ADC_CHANNEL_1: // voltage
                        {
                            voltage = ALPHA_VOLTAGE * sample_data->type2.data * 3.3 / 4096.0 * 14.7 / 4.7 + (1.0-ALPHA_VOLTAGE) * voltage;
                        }
                        break;
                    //default:
                    }
                }
                ESP_LOGD(TAG, "Voltage:%.2fV Current:%.2fA",voltage,current);
            }
        }    
    }

    float get_voltage_V()
    {
        return voltage;
    }

    float get_current_A()
    {
        return current;
    }

} // namespace POWER
