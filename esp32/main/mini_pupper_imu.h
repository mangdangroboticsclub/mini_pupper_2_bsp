/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_imu_h
#define _mini_pupper_imu_h

//#define _IMU_BY_I2C_BUS
#define _IMU_BY_SPI_BUS

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_timer.h>
#ifdef _IMU_BY_I2C_BUS

#else
  #include "driver/spi_master.h"
#endif

#include <stdint.h>

#include "mini_pupper_stats.h"

void IMU_TASK(void * parameters);
void IRAM_ATTR IMU_ISR(void * arg);

struct IMU
{
  IMU();

  uint8_t init();

  void start();

  /* DEBUG */

  uint8_t who_am_i();
  uint8_t revision();
  uint8_t read_6dof();

  /* DEBUG */

  float ax, ay, az;
  float gx, gy, gz;
  
  // public stats
    mini_pupper::periodic_process_monitor p_monitor;
    mini_pupper::frame_error_rate_monitor f_monitor;

private:

#ifdef _IMU_BY_I2C_BUS

#else
  spi_device_handle_t _spi_device_handle;
#endif

  // bus helpers
  uint8_t write_byte(uint8_t reg_addr, uint8_t data);
  uint8_t read_byte(uint8_t reg_addr, uint8_t & data);
  uint8_t read_bytes(uint8_t reg_addr, uint8_t data[], uint8_t size);

  // background host serial bus service
  TaskHandle_t _task_handle {NULL};    
  QueueHandle_t _INT2_evt_queue {NULL};
  friend void IMU_TASK(void * parameters);
  friend void IMU_ISR(void * arg);
};

extern IMU imu;

#endif //  _mini_pupper_imu_h
