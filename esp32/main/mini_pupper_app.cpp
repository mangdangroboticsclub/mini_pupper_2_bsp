/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_timer.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "mini_pupper_app.h"
#include "mini_pupper_cmd.h"
#include "mini_pupper_servos.h"
#include "mini_pupper_host.h"
#include "mini_pupper_imu.h"
#include "mini_pupper_power.h"

static const char* TAG = "MAIN";

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 4,
	    .allocation_unit_size = 1024,
	    .disk_status_check_enable = false
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

e_mini_pupper_state state {STATE_IDLE};

extern "C" void app_main(void)
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "mini_pupper>";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    initialize_nvs();

    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
    ESP_LOGI(TAG, "Command history enabled");

    /* Register commands */
    esp_console_register_help_command();
    register_mini_pupper_cmds();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

#else
#error Unsupported console type
#endif

    // start CLI early
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    // init IMU device
    uint8_t const imu_status = imu.init();
    if(imu_status==0)
        ESP_LOGI(TAG, "IMU device init [OK].");
    else
        ESP_LOGI(TAG, "IMU device configuration [FAILURE] (error:%d)!",imu_status);

    // start IMU service
    imu.start();
    ESP_LOGI(TAG, "IMU service started.");

    // start POWER service
    POWER::start();
    ESP_LOGI(TAG, "Power service started.");

    // start HOST interface, service disabled at start-up (servo setpoint ignored, servo feedback nok, IMU feedback ok, POWER feedback ok)
    host.start();
    ESP_LOGI(TAG, "Host communication service started, but disabled.");

    // start SERVO task, SERVO not powered, service disabled at start-up.
    servo.start();
    ESP_LOGI(TAG, "Servo control & feedback service started, but disabled.");

    // read calibration data from flash
    {
        // save to flash
        FILE * f = fopen(CALIBRATE_PATH, "r");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open calibration file for reading");
            ESP_LOGI(TAG, "Default calibration : 0 0 0 0 0 0 0 0 0 0 0 0");
            // apply zero offset
            servo.resetCalibration();
        }
        if(f)
        {
            s16 servoOffsets[12] {0};
            for(size_t index=0; index<12; ++ index)
            {
                int data {0};
                fscanf(f,"%d\n", &data);
                servoOffsets[index] = data;
            }
            fclose(f);
            ESP_LOGI(TAG, "Calibration read : %d %d %d %d %d %d %d %d %d %d %d %d",
                servoOffsets[0],servoOffsets[1],servoOffsets[2],
                servoOffsets[3],servoOffsets[4],servoOffsets[5],
                servoOffsets[6],servoOffsets[7],servoOffsets[8],
                servoOffsets[9],servoOffsets[10],servoOffsets[11]
            );            
            // apply offset
            servo.setCalibration(servoOffsets);
        }
    }    

    // robot loop
    ESP_LOGI(TAG, "STATE_IDLE.");
    int const low_voltage_cutoff_counter_max {60}; // 1 minute
    int low_voltage_cutoff_counter {low_voltage_cutoff_counter_max}; 
    float const low_voltage_threshold_V {6.2}; // V
    float const normal_voltage_threshold_V {6.6}; // V
    int64_t last_time = esp_timer_get_time();
    u8 const servoTorquesOFF[12] {0};
    u8 const servoTorquesON[12] {1,1,1,1,1,1,1,1,1,1,1,1};
    for(;;)
    {
        // slow down this background task (100Hz refresh rate)
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // heart-beat
        ESP_LOGD(TAG, ".");

        switch(state)
        {
        default:
        case STATE_IDLE:
            // Mini pupper responds to HOST and CLI. 
            // IMU is working. 
            // SERVO are powered OFF. 
            // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback nOK, IMU feedback ok, POWER feedback ok)
            {
                // force SERVO power disable
                servo.enable_power(false);

                // force HOST service disable
                host.enable_service(false);

                // stand-up if battery voltage is ok
                if(POWER::get_voltage_V()>normal_voltage_threshold_V)
                {
                    state = STATE_STANDING_UP;
                    ESP_LOGI(TAG, "STATE_STANDING_UP.");
                    last_time = esp_timer_get_time();
                    // enable SERVO power and wait a little while
                    servo.enable_power();
                    ESP_LOGI(TAG, "Servo power supply enabled.");
                    servo.enable_service();
                    ESP_LOGI(TAG, "Servo service enabled.");
                    // force torque disable
                    servo.setTorque12Async(servoTorquesOFF);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    // set goal position equal to present position
                    u16 servoPositions[12] {0};
                    servo.getPosition12Async(servoPositions);   
                    servo.setPosition12Async(servoPositions);
                    // set torque enable
                    servo.setTorque12Async(servoTorquesON);  
                    // reset cut-off 
                    low_voltage_cutoff_counter = low_voltage_cutoff_counter_max; 
                }
            }
            break;

        case STATE_STANDING_UP:
            // Mini pupper stand up in a few seconds.
            // IMU is working. 
            // SERVO are powered ON.
            // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback OK, IMU feedback ok, POWER feedback ok)
            {
                // TODO
                // note : required a servo stand-by position (not necessary neutral position)

                // update positions to stand-by position
                u16 servoPositions[12] {0};
                servo.getGoalPosition12Async(servoPositions);                   
                u16 const speed = 2;
                for(auto & position : servoPositions)
                {
                    if(position<512)
                        position += speed;
                    else if(position>512)
                        position -= speed;
                }
                servo.setPosition12Async(servoPositions);
                servo.setTorque12Async(servoTorquesON);   
                
                // one leg debug
                ESP_LOGD(TAG, "Servo#0 pos:%d",servoPositions[0]);
                ESP_LOGD(TAG, "Servo#1 pos:%d",servoPositions[1]);
                ESP_LOGD(TAG, "Servo#2 pos:%d",servoPositions[2]);

                // go to walking state when standing-up has finished
                if(esp_timer_get_time()>last_time+4000000)
                {
                    state = STATE_WALKING;
                    ESP_LOGI(TAG, "STATE_WALKING.");
                    // HOST service enable
                    host.enable_service();
                    ESP_LOGI(TAG, "Host service enabled.");
                }
            }
            break;

        case STATE_WALKING:
            // Mini pupper can walk. 
            // IMU is working. 
            // SERVO are powered ON.
            // HOST service is enabled (torque switch and goal position setpoint are taken in account, SERVO feedback OK, IMU feedback ok, POWER feedback ok)
            {

                // HOST is controling SERVO

                // low-voltage cutoff
                if(POWER::get_voltage_V()<low_voltage_threshold_V && low_voltage_cutoff_counter>0)
                {
                    --low_voltage_cutoff_counter;
                    ESP_LOGI(TAG, "Warning : Low-voltage!");            
                }
                if(POWER::get_voltage_V()>normal_voltage_threshold_V && low_voltage_cutoff_counter!=0 && low_voltage_cutoff_counter<low_voltage_cutoff_counter_max)
                {
                    ++low_voltage_cutoff_counter;
                }
                if(low_voltage_cutoff_counter==0)
                {
                    state = STATE_SITTING_DOWN;
                    ESP_LOGI(TAG, "STATE_SITTING_DOWN.");                    
                }


            }
            break;

        case STATE_CALIBRATION:
            // Mini pupper is saving calibration.
            // IMU is working.
            // SERVO are powered OFF.
            {
                // force SERVO power disable
                servo.enable_power(false);
            }
            break;

        case STATE_SITTING_DOWN:
            // Mini pupper sit down in a few seconds.
            // IMU is working. 
            // SERVO are powered ON.
            // HOST service is disabled (torque switch and goal position setpoint are ignored, SERVO feedback OK, IMU feedback ok, POWER feedback ok)
            {
                // TODO
                // TODO
                // TODO
                // TODO
                // note : required a servo rest position (not necessary neutral position)

                state = STATE_RESTING;
                ESP_LOGI(TAG, "STATE_RESTING.");                  

                servo.enable_power(false);
                ESP_LOGI(TAG, "Servo power supply disabled (low-voltage cutoff)!");            

            }
            break;

        case STATE_RESTING:
            {
                // force SERVO power disable
                servo.enable_power(false);

                // force HOST service disable
                host.enable_service(false);

            }
            break;

        }
    }
}
