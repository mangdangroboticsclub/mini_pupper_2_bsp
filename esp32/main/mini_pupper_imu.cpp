/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#include "mini_pupper_imu.h"
#include "mini_pupper_tasks.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#ifdef _IMU_BY_I2C_BUS
  #include "driver/i2c.h"
#else
  #include "driver/spi_master.h"
#endif

#include <cmath>
#include <cstring>

static const char *TAG = "IMU";

#ifdef _IMU_BY_I2C_BUS
    #define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
    #define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
    #define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
    #define I2C_MASTER_SDA_IO           41
    #define I2C_MASTER_SCL_IO           42
    #define I2C_MASTER_TIMEOUT_MS       1000
    #define I2C_MASTER_NUM              I2C_NUM_0
    #define I2C_DEV_ADDR (uint8_t)0x6B //0b1101010+1b (A0=GND)
#else
    #define SPI_MASTER_ID   SPI2_HOST
    #define SPI_MASTER_MISO 13
    #define SPI_MASTER_MOSI 11
    #define SPI_MASTER_CLK  12
    #define SPI_MASTER_CS   38
    #define SPI_READ     (0x80)  /*!< addr | SPIBUS_READ  */
    #define SPI_WRITE    (0x7F)  /*!< addr & SPIBUS_WRITE */
#endif

/** registers */
#define QMI8658C_WHO_AM_I_REG               0x00 // ID in QMI8658C default to 0x05
#define QMI8658C_REVISION                   0x01 
#define QMI8658C_ACC_GYRO_CTRL1_SPI_REG     0x02
#define QMI8658C_ACC_GYRO_CTRL2_ACC_REG			0x03
#define QMI8658C_ACC_GYRO_CTRL3_G_REG			  0x04
#define QMI8658C_ACC_GYRO_CTRL4_M_REG			  0x05
#define QMI8658C_ACC_GYRO_CTRL5_REG			    0x06
#define QMI8658C_ACC_GYRO_CTRL6_AE_SET			0x07
#define QMI8658C_ACC_GYRO_CTRL7_REG			    0x08
#define QMI8658C_ACC_GYRO_CTRL9_REG			    0x0A
#define QMI8658C_ACC_GYRO_CAL1_L_REG			  0x0B
#define QMI8658C_ACC_GYRO_CTRL9_HOST_REG		0x10

#define QMI8658C_STATUSINT_REG		          0x2D
#define QMI8658C_STATUS0_REG		            0x2E

#define QMI8658C_ACC_GYRO_AE_REG1		        0x57
#define QMI8658C_ACC_GYRO_AE_REG2		        0x58

#define QMI8658C_ACC_GYRO_RESET		          0x60

#define QMI8658C_ACC_GYRO_OUT_L_TEMP_REG		0x33
#define QMI8658C_ACC_GYRO_OUT_H_TEMP_REG		0x34

#define QMI8658C_ACC_GYRO_OUTX_L_XL_REG			0x35
#define QMI8658C_ACC_GYRO_OUTX_H_XL_REG			0x36
#define QMI8658C_ACC_GYRO_OUTY_L_XL_REG			0x37
#define QMI8658C_ACC_GYRO_OUTY_H_XL_REG			0x38
#define QMI8658C_ACC_GYRO_OUTZ_L_XL_REG			0x39
#define QMI8658C_ACC_GYRO_OUTZ_H_XL_REG			0x3A

#define QMI8658C_ACC_GYRO_OUTX_L_G_REG			0x3B
#define QMI8658C_ACC_GYRO_OUTX_H_G_REG			0x3C
#define QMI8658C_ACC_GYRO_OUTY_L_G_REG			0x3D
#define QMI8658C_ACC_GYRO_OUTY_H_G_REG			0x3E
#define QMI8658C_ACC_GYRO_OUTZ_L_G_REG			0x3F
#define QMI8658C_ACC_GYRO_OUTZ_H_G_REG			0x40

#define QMI8658C_ACC_GYRO_OUTX_L_M_REG			0x41
#define QMI8658C_ACC_GYRO_OUTX_H_M_REG			0x42
#define QMI8658C_ACC_GYRO_OUTY_L_M_REG			0x43
#define QMI8658C_ACC_GYRO_OUTY_H_M_REG			0x44
#define QMI8658C_ACC_GYRO_OUTZ_L_M_REG			0x45
#define QMI8658C_ACC_GYRO_OUTZ_H_M_REG			0x46

#define QMI8658C_ACC_GYRO_OUTW_L_Q_REG			0x49
#define QMI8658C_ACC_GYRO_OUTW_H_Q_REG			0x4A
#define QMI8658C_ACC_GYRO_OUTX_L_Q_REG			0x4B
#define QMI8658C_ACC_GYRO_OUTX_H_Q_REG			0x4C
#define QMI8658C_ACC_GYRO_OUTY_L_Q_REG			0x4D
#define QMI8658C_ACC_GYRO_OUTY_H_Q_REG			0x4E
#define QMI8658C_ACC_GYRO_OUTZ_L_Q_REG			0x4F
#define QMI8658C_ACC_GYRO_OUTZ_H_Q_REG			0x50

#define QMI8658C_ACC_GYRO_OUTX_L_V_REG			0x51
#define QMI8658C_ACC_GYRO_OUTX_H_V_REG			0x52
#define QMI8658C_ACC_GYRO_OUTY_L_V_REG			0x53
#define QMI8658C_ACC_GYRO_OUTY_H_V_REG			0x54
#define QMI8658C_ACC_GYRO_OUTZ_L_V_REG			0x55
#define QMI8658C_ACC_GYRO_OUTZ_H_V_REG			0x56

IMU imu;

IMU::IMU():
_task_handle(NULL) 
{
#ifdef _IMU_BY_I2C_BUS
  // start i2c bus
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_MASTER_SDA_IO;
  conf.scl_io_num = I2C_MASTER_SCL_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
  conf.clk_flags = 0;
  i2c_param_config(I2C_NUM_0, &conf);

  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
  ESP_LOGI(TAG, "I2C initialized successfully");
#else
    // start SPI host
    spi_bus_config_t bus_cfg = {
      .mosi_io_num=SPI_MASTER_MOSI,
      .miso_io_num=SPI_MASTER_MISO,
      .sclk_io_num=SPI_MASTER_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 128
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_MASTER_ID, &bus_cfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI host initialized successfully");

    // configure SPI device (IMU) on the other side of the bus
    spi_device_interface_config_t dev_cfg = {
        .command_bits=0,
        .address_bits=8,
        .dummy_bits=0,
        .mode=0,                                
        .duty_cycle_pos=128,
        .cs_ena_pretrans=0,
        .cs_ena_posttrans=0,
        .clock_speed_hz=12*1000000, // 12MHz (15MHz max)
        .input_delay_ns = 0,
        .spics_io_num=SPI_MASTER_CS,            
        .flags = 0,
        .queue_size=2,
        .pre_cb = 0,
        .post_cb = 0
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_MASTER_ID, &dev_cfg, &_spi_device_handle));
    ESP_LOGI(TAG, "SPI device initialized successfully");

#endif
  // GPIO #39 configuration (IMU :: INT2)  
  gpio_config_t io_conf {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_39);
  gpio_config(&io_conf);
}

struct imu_configuration
{
  uint8_t reg;
  uint8_t value;
};

uint8_t IMU::init()
{
  ESP_LOGI(TAG, "chip identifier: %02xh",(int)who_am_i());
  ESP_LOGI(TAG, "chip revision: %02xh",(int)revision());

  imu_configuration const configuration[] = {
    {QMI8658C_ACC_GYRO_CTRL1_SPI_REG, 0b01100000 }, // 0b01100000 address auto increment +  read data little endian + sensor enable
    {QMI8658C_ACC_GYRO_CTRL7_REG,     0b00000011 }, // 0b11001011 6D AE mode : enable gyro + enable acc
    {QMI8658C_ACC_GYRO_CTRL2_ACC_REG, 0b00000101 }, // 0b00000101 2g aODR = 235Hz
    {QMI8658C_ACC_GYRO_CTRL3_G_REG,   0b01110101 }  // 0b01110101 2048dps gODR = 235Hz
  };
  uint8_t read_value {0};
  for(auto const & config : configuration)
  {
    if(write_byte(config.reg,config.value)) return config.reg;
    //vTaskDelay(1 / portTICK_PERIOD_MS);
    if(read_byte(config.reg, read_value)) return config.reg; 
    if(read_value!=config.value) return config.reg;
    //vTaskDelay(1 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "configure register: %02xh - value: %02xh",(int)config.reg,(int)config.value);
  }
  return 0;
}

uint8_t IMU::write_byte(uint8_t reg_addr, uint8_t data)
{
#ifdef _IMU_BY_I2C_BUS
  uint8_t write_buf[2] = {reg_addr, data};
  return i2c_master_write_to_device(I2C_MASTER_NUM, I2C_DEV_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
#else
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); 
  t.flags = 0;
  t.user = nullptr;
  t.cmd = 0;
  t.addr = reg_addr & SPI_WRITE;
  t.length=1*8;
  t.rxlength=0;
  uint8_t buffer[1] = {data}; // TODO : use TX DATA
  t.tx_buffer = buffer;
  t.rx_buffer = nullptr;
  ESP_LOGD(TAG, "write_byte : %d %d",(int)t.addr,(int)buffer[0]);
  return spi_device_transmit(_spi_device_handle, &t);

#endif
}

uint8_t IMU::read_byte(uint8_t reg_addr, uint8_t & data)
{
#ifdef _IMU_BY_I2C_BUS
  return i2c_master_write_read_device(I2C_MASTER_NUM, I2C_DEV_ADDR, &reg_addr, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
#else
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); 
  t.flags = 0;
  t.user = nullptr;
  t.cmd = 0;  
  t.addr = reg_addr | SPI_READ; 
  t.length=1*8;
  t.rxlength=1*8;
  t.tx_buffer=nullptr;   
  t.rx_buffer = &data;    // TODO : use RX DATA
  esp_err_t err = spi_device_transmit(_spi_device_handle, &t);
  ESP_LOGD(TAG, "read_byte : %d %d",(int)t.addr,(int)data);
  return err;
#endif
}

uint8_t IMU::read_bytes(uint8_t reg_addr, uint8_t data[], uint8_t size)
{
#ifdef _IMU_BY_I2C_BUS
  return i2c_master_write_read_device(I2C_MASTER_NUM, I2C_DEV_ADDR, &reg_addr, 1, data, size, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
#else
  spi_transaction_t t;
  memset(&t, 0, sizeof(t)); 
  t.flags = 0;
  t.user=nullptr;
  t.cmd = 0;
  t.addr = reg_addr | SPI_READ;
  t.length=size*8;
  t.rxlength=size*8;
  t.tx_buffer=nullptr;   
  t.rx_buffer=data;   
  return spi_device_transmit(_spi_device_handle, &t);
#endif
}

uint8_t IMU::read_6dof()
{
  uint8_t raw[12] {0};
  uint8_t const reg_addr {QMI8658C_ACC_GYRO_OUTX_L_XL_REG};

#ifdef _IMU_BY_I2C_BUS
  uint8_t err = i2c_master_write_read_device(I2C_MASTER_NUM, I2C_DEV_ADDR, &reg_addr, 1, raw, sizeof(raw), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
#else
  uint8_t err = read_bytes(reg_addr,raw,12);
#endif

  if(!err)
  {
    ax = 1.0f/16384.0f*((int16_t)(raw[1]<<8) | raw[0]);
    ay = 1.0f/16384.0f*((int16_t)(raw[3]<<8) | raw[2]);
    az = 1.0f/16384.0f*((int16_t)(raw[5]<<8) | raw[4]);
    gx = 1.0f/16.0f* ((int16_t)(raw[7]<<8) | raw[6]);
    gy = 1.0f/16.0f* ((int16_t)(raw[9]<<8) | raw[8]);
    gz = 1.0f/16.0f* ((int16_t)(raw[11]<<8) | raw[10]);
    // log debug
    ESP_LOGD(TAG, "ax:%0.3f ay:%0.3f az:%0.3f gx:%0.3f gy:%0.3f gz:%0.3f", ax, ay, az, gx, gy, gz );    
    // stats
    f_monitor.update();
    return 0;
  }
  else
  {
    ax = 0.0f;
    ay = 0.0f;
    az = 0.0f;
    gx = 0.0f;
    gy = 0.0f;
    gz = 0.0f;
    // stats
    f_monitor.update(mini_pupper::frame_error_rate_monitor::TIME_OUT_ERROR);
    return err;
  }
}

uint8_t IMU::who_am_i()
{
  uint8_t read_value {0};
  read_byte(QMI8658C_WHO_AM_I_REG, read_value);
  return read_value;
}

uint8_t IMU::revision()
{
  uint8_t read_value;
  read_byte(QMI8658C_WHO_AM_I_REG+1, read_value);
  return read_value;
}

void IMU_ISR(void* arg)
{
    IMU * imu = reinterpret_cast<IMU*>(arg);
    uint32_t const value {39};
    xQueueSendFromISR(imu->_INT2_evt_queue, &value, NULL);
}

static size_t const stack_size = 10000;
static StackType_t stack[stack_size] {0};
static StaticTask_t task_buffer;

void IMU::start()
{
  //create a queue to handle gpio event from isr
  _INT2_evt_queue = xQueueCreate(20, sizeof(uint32_t));

  // start the task
  _task_handle = xTaskCreateStaticPinnedToCore(
      IMU_TASK,   
      "IMU SERVICE",
      stack_size,         
      (void*)this,        
      IMU_PRIORITY,     
      stack,
      &task_buffer,
      IMU_CORE
  );

  //change gpio interrupt type for one pin
  gpio_set_intr_type(GPIO_NUM_39, GPIO_INTR_POSEDGE);

  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);

  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(GPIO_NUM_39, IMU_ISR, (void*)this);
}

void IMU_TASK(void * parameters)
{
  IMU * imu { reinterpret_cast<IMU*>(parameters) };
  for(;;)
  {
    // Waiting for UART event.
    static uint32_t value {0};
    if(!xQueueReceive(imu->_INT2_evt_queue,(void*)&value,(TickType_t)portMAX_DELAY)) continue;
    xQueueReset(imu->_INT2_evt_queue);

    // Read raw IMU data
    uint8_t const read_data { imu->read_6dof() };
    if(read_data)
      ESP_LOGI(TAG, "IMU read error!!!");

    // log debug
    ESP_LOGD(TAG, "ax:%0.3f ay:%0.3f az:%0.3f gx:%0.3f gy:%0.3f gz:%0.3f", imu->ax, imu->ay, imu->az, imu->gx, imu->gy, imu->gz );

    // stats
    imu->p_monitor.update();
  }    
}
