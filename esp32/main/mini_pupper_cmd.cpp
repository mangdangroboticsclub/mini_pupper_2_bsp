#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "cmd_system.h"
#include "cmd_wifi.h"

#include "mini_pupper_imu_cmd.h"
#include "mini_pupper_servos.h"
#include "mini_pupper_host.h"
#include "mini_pupper_imu.h"
#include "mini_pupper_app.h"
#include "mini_pupper_cmd.h"
#include "mini_pupper_ota.h"

static const char *TAG = "CLI";

static struct
{
    struct arg_int *servo_id;
    struct arg_end *end;
} servo_id_args;

static struct
{
    struct arg_int *servo_id;
    struct arg_int *loop;
    struct arg_end *end;
} servo_id_loop_args;

static struct
{
    struct arg_int *servo_pos12;
    struct arg_end *end;
} servo_pos12_args;

/*
 * Switch ON/OFF servo power supply
 *
 */

static int mini_pupper_cmd_disable(int argc, char **argv)
{
    servo.enable_power(false);
    return 0;
}

static void register_mini_pupper_cmd_disable(void)
{
    const esp_console_cmd_t cmd_servo_disable = {
        .command = "servo-disable",
        .help = "disabing the servos",
        .hint = NULL,
        .func = &mini_pupper_cmd_disable,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_disable) );
}

static int mini_pupper_cmd_enable(int argc, char **argv)
{
    servo.enable_power();
    return 0;
}

static void register_mini_pupper_cmd_enable(void)
{
    const esp_console_cmd_t cmd_servo_enable = {
        .command = "servo-enable",
        .help = "enabing the servos",
        .hint = NULL,
        .func = &mini_pupper_cmd_enable,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_enable) );
}

static int mini_pupper_cmd_isEnabled(int argc, char **argv)
{
    if(servo.is_power_enabled()) {
        printf("servos are enabled\r\n");
    }
    else{
        printf("servos are not enabled\r\n");
    }
    return 0;
}

static void register_mini_pupper_cmd_isEnabled(void)
{
    const esp_console_cmd_t cmd_servo_isEnabled = {
        .command = "servo-isEnabled",
        .help = "check if the servos are enabled",
        .hint = NULL,
        .func = &mini_pupper_cmd_isEnabled,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_isEnabled) );
}

/*
 * Switch ON/OFF servo torque
 *
 */

static int mini_pupper_cmd_disableTorque(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 && servo_id != 254 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id == 254) {
        printf("Warning: your servo ID is the broadcast ID\r\n");
    }
    servo.disable_torque((u8)servo_id);
    return 0;
}

static void register_mini_pupper_cmd_disableTorque(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo = {
        .command = "servo-disableTorque",
        .help = "disable torque",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_disableTorque,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo) );
}


static int mini_pupper_cmd_enableTorque(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 && servo_id != 254 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id == 254) {
        printf("Warning: your servo ID is the broadcast ID\r\n");
    }
    servo.enable_torque((u8)servo_id);
    return 0;
}

static void register_mini_pupper_cmd_enableTorque(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo = {
        .command = "servo-enableTorque",
        .help = "enable torque",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_enableTorque,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo) );
}


static int mini_pupper_cmd_isTorqueEnabled(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    u8 enable {0};
    servo.is_torque_enable((u8)servo_id,enable);
    if(enable) {
        printf("servos torque is enabled\r\n");
    }
    else{
        printf("servos torque is not enabled\r\n");
    }
    return 0;
}

static void register_mini_pupper_cmd_isTorqueEnabled(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo = {
        .command = "servo-isTorqueEnable",
        .help = "check torque switch",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_isTorqueEnabled,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo) );
}

/*
 * Ping servo
 *
 */

static int mini_pupper_cmd_scan(int argc, char **argv)
{
    printf("Servos on the bus:\r\n");
    for(u8 i = 1; i<13; i++)
    {
        if(servo.ping(i) == SERVO_STATUS_OK)
        {
            printf("%d ", i);
	   }
    }
    printf("\r\n");
    return 0;
}

static void register_mini_pupper_cmd_scan(void)
{
    const esp_console_cmd_t cmd_servo_scan = {
        .command = "servo-scan",
        .help = "scan the bus for servos",
        .hint = NULL,
        .func = &mini_pupper_cmd_scan,
	   .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_scan) );
}

static int mini_pupper_cmd_extended_menu(int argc, char **argv)
{
    register_mini_pupper_extended_cmds();
    return 0;
}

static void register_mini_pupper_cmd_extended_menu(void)
{
    const esp_console_cmd_t cmd_extended_menu = {
        .command = "extended-menu",
        .help = "enable extended menu",
        .hint = NULL,
        .func = &mini_pupper_cmd_extended_menu,
           .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_extended_menu) );
}

static int mini_pupper_cmd_calibrate_begin(int argc, char **argv)
{
    state = STATE_CALIBRATION_START;
    return 0;
}

static void register_mini_pupper_cmd_calibrate_begin(void)
{
    const esp_console_cmd_t cmd_calibrate = {
        .command = "calibrate-begin",
        .help = "begin mini pupper calibration",
        .hint = NULL,
        .func = &mini_pupper_cmd_calibrate_begin,
           .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_calibrate) );
}

static int mini_pupper_cmd_calibrate_end(int argc, char **argv)
{
    state = STATE_CALIBRATION_FINISH;
    return 0;
}

static void register_mini_pupper_cmd_calibrate_end(void)
{
    const esp_console_cmd_t cmd_calibrate = {
        .command = "calibrate-end",
        .help = "end mini pupper calibration",
        .hint = NULL,
        .func = &mini_pupper_cmd_calibrate_end,
           .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_calibrate) );
}

static int mini_pupper_cmd_calibrate_clear(int argc, char **argv)
{
  if (remove("/data//calib.txt") == 0) {
      printf("The calibration file is deleted successfully.");
      servo.resetCalibration();
  } else {
      printf("The calibration file is not deleted.");
  }
  return 0;
}

static void register_mini_pupper_cmd_calibrate_clear(void)
{
    const esp_console_cmd_t cmd_calibrate = {
        .command = "calibrate-clear",
        .help = "clear mini pupper calibration",
        .hint = NULL,
        .func = &mini_pupper_cmd_calibrate_clear,
           .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_calibrate) );
}

static struct {
    struct arg_str *ssid;
    struct arg_str *wifi_passwd;
    struct arg_end *end;
} ota_args;


static int mini_pupper_cmd_ota(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&ota_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ota_args.end, argv[0]);
        return 0;
    }

    // check for OTA updates
    state = STATE_RESTING;
    check_ota_updates(ota_args.ssid->sval[0], ota_args.wifi_passwd->sval[0]);
  return 0;
}

static void register_mini_pupper_cmd_ota(void)
{
    ota_args.ssid = arg_str1(NULL, "ssid", "<ssid>", "Wifi SSID");
    ota_args.wifi_passwd = arg_str0(NULL, "password", "<pwd>", "Wifi password");
    ota_args.end = arg_end(2);
    const esp_console_cmd_t cmd_ota = {
        .command = "update",
        .help = "Get OTA updates",
        .hint = "--ssid <ssid> --password <pwd>",
        .func = &mini_pupper_cmd_ota,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_ota) );
}

static struct {
    struct arg_int *servo_id;
    struct arg_int *servo_pos;
    struct arg_end *end;
} servo_pos_args;

static int mini_pupper_cmd_setPosition(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_pos_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_pos_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_pos_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 && servo_id != 254 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id == 254) {
        printf("Warning: your servo ID is the broadcast ID\r\n");
    }

    /* Check servo_pos "--pos" option */
    int servo_pos = servo_pos_args.servo_pos->ival[0];
    if(servo_pos<0 || servo_pos>1023) {
        printf("Invalid servo position\r\n");
        return 0;
    }
    servo.set_goal_position((u8)servo_id, (u16)servo_pos);
    return 0;
}

static void register_mini_pupper_cmd_setPosition(void)
{
    servo_pos_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_pos_args.servo_pos = arg_int1(NULL, "pos", "<n>", "Servo Position");
    servo_pos_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_setPosition = {
        .command = "servo-setPosition",
        .help = "rotate the servos to a given position",
        .hint = "--id <servoID> --pos <position>",
        .func = &mini_pupper_cmd_setPosition,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_setPosition) );
}

static int mini_pupper_cmd_setPosition12(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_pos12_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_pos_args.end, argv[0]);
        return 0;
    }
    static u16 servoPositions[12] {0};
    for(size_t index=0;index<12;++index) {
        int const & value = servo_pos12_args.servo_pos12->ival[index];
        if(0<=value && value<1024)
            servoPositions[index]=static_cast<u16>(value);
        else
            servoPositions[index]=512;
    }
    servo.set_position_all(servoPositions);
    return 0;
}

static void register_mini_pupper_cmd_setPosition12(void)
{
    servo_pos12_args.servo_pos12 = arg_intn(NULL,NULL,"<pos>",12,12,"Servo position array (x12)");
    servo_pos12_args.end = arg_end(1);
    const esp_console_cmd_t cmd_servo_setPosition12 = {
        .command = "servo-setPosition12",
        .help = "rotate the servos to a given position",
        .hint = "<pos> (x12)",
        .func = &mini_pupper_cmd_setPosition12,
        .argtable = &servo_pos12_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_setPosition12) );
}

static struct {
    struct arg_int *servo_id;
    struct arg_int *servo_newid;
    struct arg_end *end;
} servo_newid_args;

static int mini_pupper_cmd_setID(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_newid_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_newid_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_newid_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }

    /* Check servo_newid "--newid" option */
    int servo_newid = servo_newid_args.servo_newid->ival[0];
    if(servo_newid<0 || servo_newid>12) {
        printf("Invalid new servo ID\r\n");
        return 0;
    }
    servo.setID((u8)servo_id, (u8)servo_newid);
    return 0;
}

static void register_mini_pupper_cmd_setID(void)
{
    servo_newid_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_newid_args.servo_newid = arg_int1(NULL, "newid", "<n>", "New Servo ID");
    servo_newid_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_setID = {
        .command = "servo-setID",
        .help = "Change the servo ID",
        .hint = "--id <servoID> --newid <new servoID>",
        .func = &mini_pupper_cmd_setID,
	   .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_setID) );
}

static int mini_pupper_cmd_getPosition(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        u16 position {0};
        servo.get_position((u8)servo_id, position);
        printf("Pos: %d\r\n", position);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getPosition(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo = {
        .command = "servo-getPosition",
        .help = "return position",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getPosition,
        .argtable = &servo_id_loop_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo) );
}

static int mini_pupper_cmd_getSpeed(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        s16 speed {0};
        servo.get_speed((u8)servo_id,speed);
        printf("Speed: %d\r\n", speed);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getSpeed(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getSpeed = {
        .command = "servo-getSpeed",
        .help = "return speed",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getSpeed,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getSpeed) );
}

static int mini_pupper_cmd_getLoad(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        s16 load {0};
        servo.get_load((u8)servo_id,load);
        printf("Load: %d\r\n", load);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getLoad(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getLoad = {
        .command = "servo-getLoad",
        .help = "return load",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getLoad,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getLoad) );
}

static int mini_pupper_cmd_getVoltage(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        u8 voltage {0};
        servo.get_voltage((u8)servo_id,voltage);
        printf("Voltage: %d\r\n", voltage);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getVoltage(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getVoltage = {
        .command = "servo-getVoltage",
        .help = "return voltage",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getVoltage,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getVoltage) );
}

static int mini_pupper_cmd_getTemperature(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        u8 temperature {0};
        servo.get_temperature((u8)servo_id,temperature);
        printf("Temp: %d\r\n", temperature);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getTemperature(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getTemperature = {
        .command = "servo-getTemperature",
        .help = "return temperature",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getTemperature,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getTemperature) );
}

static int mini_pupper_cmd_getMove(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        u8 move {0};
        servo.get_move((u8)servo_id,move);
        printf("Move: %d\r\n", move);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getMove(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getMove = {
        .command = "servo-getMove",
        .help = "return move",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getMove,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getMove) );
}

static int mini_pupper_cmd_getCurrent(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_loop_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_loop_args.servo_id->ival[0];
    int loop = servo_id_loop_args.loop->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    for(int i=0; i<loop; i++){
        s16 current {0};
        servo.get_current((u8)servo_id,current);
        printf("Current: %d\r\n", current);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_mini_pupper_cmd_getCurrent(void)
{
    servo_id_loop_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    servo_id_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getCurrent = {
        .command = "servo-getCurrent",
        .help = "return current",
        .hint = "--id <servoID> --loop <n>",
        .func = &mini_pupper_cmd_getCurrent,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getCurrent) );
}

static int mini_pupper_cmd_setPositionAsync(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_pos_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_pos_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_pos_args.servo_id->ival[0];
    if(servo_id<0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 && servo_id != 254 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id == 254) {
        printf("Warning: your servo ID is the broadcast ID\r\n");
    }

    /* Check servo_pos "--pos" option */
    int servo_pos = servo_pos_args.servo_pos->ival[0];
    if(servo_pos<0 || servo_pos>1023) {
        printf("Invalid servo position\r\n");
        return 0;
    }
    servo.setPositionAsync((u8)servo_id, (u16)servo_pos);
    return 0;
}

static void register_mini_pupper_cmd_setPositionAsync(void)
{
    servo_pos_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_pos_args.servo_pos = arg_int1(NULL, "pos", "<n>", "Servo Position");
    servo_pos_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_setPosition = {
        .command = "servo-setPositionAsync",
        .help = "rotate the servos to a given position",
        .hint = "--id <servoID> --pos <position>",
        .func = &mini_pupper_cmd_setPositionAsync,
        .argtable = &servo_pos_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_setPosition) );
}


static int mini_pupper_cmd_setPosition12Async(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_pos12_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_pos_args.end, argv[0]);
        return 0;
    }
    static u16 servoPositions[12] {0};
    for(size_t index=0;index<12;++index) {
        int const & value = servo_pos12_args.servo_pos12->ival[index];
        if(0<=value && value<1024)
            servoPositions[index]=static_cast<u16>(value);
        else
            servoPositions[index]=512;
    }
    servo.setPosition12Async(servoPositions);
    return 0;
}

static void register_mini_pupper_cmd_setPosition12Async(void)
{
    servo_pos12_args.servo_pos12 = arg_intn(NULL,NULL,"<pos>",12,12,"Servo position array (x12)");
    servo_pos12_args.end = arg_end(1);
    const esp_console_cmd_t cmd_servo_setPosition12 = {
        .command = "servo-setPosition12Async",
        .help = "rotate the servos to a given position",
        .hint = "<pos> (x12)",
        .func = &mini_pupper_cmd_setPosition12Async,
        .argtable = &servo_pos12_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_setPosition12) );
}

static int mini_pupper_cmd_getPositionAsync(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<=0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    printf("Pos: %d\r\n", servo.getPositionAsync((u8)servo_id));
    return 0;
}

static void register_mini_pupper_cmd_getPositionAsync(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_ReadPos = {
        .command = "servo-getPositionAsync",
        .help = "return position",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_getPositionAsync,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_ReadPos) );
}

static int mini_pupper_cmd_getSpeedAsync(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<=0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    printf("ReadVel: %d\r\n", servo.getSpeedAsync((u8)servo_id));
    return 0;
}

static void register_mini_pupper_cmd_getSpeedAsync(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_ReadVel = {
        .command = "servo-getSpeedAsync",
        .help = "return speed",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_getSpeedAsync,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_ReadVel) );
}

static int mini_pupper_cmd_getLoadAsync(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&servo_id_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, servo_id_args.end, argv[0]);
        return 0;
    }

    /* Check servoID "--id" option */
    int servo_id = servo_id_args.servo_id->ival[0];
    if(servo_id<=0) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    if( servo_id>12 ) {
        printf("Invalid servo ID\r\n");
        return 0;
    }
    printf("Load: %d\r\n", servo.getLoadAsync((u8)servo_id));
    return 0;
}

static void register_mini_pupper_cmd_getLoadAsync(void)
{
    servo_id_args.servo_id = arg_int1(NULL, "id", "<n>", "Servo ID");
    servo_id_args.end = arg_end(2);
    const esp_console_cmd_t cmd_servo_getLoad = {
        .command = "servo-getLoadAsync",
        .help = "return load",
        .hint = "--id <servoID>",
        .func = &mini_pupper_cmd_getLoadAsync,
        .argtable = &servo_id_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getLoad) );
}


static int mini_pupper_cmd_stats(int argc, char **argv)
{
    ESP_LOGI(TAG, "HOST communication frequency: %.0fHz < %.0fHz < %.0fHz  [var:%.1fHz] (count:%lld)",
        host.p_monitor.frequency_min,
        host.p_monitor.frequency_mean,
        host.p_monitor.frequency_max,
        sqrtf(host.p_monitor.frequency_var),
        host.p_monitor.counter
    );  
    ESP_LOGI(TAG, "SERVO control frequency: %.0fHz < %.0fHz < %.0fHz  [var:%.1fHz] (count:%lld)",
        servo.p_monitor.frequency_min,
        servo.p_monitor.frequency_mean,
        servo.p_monitor.frequency_max,
        sqrtf(servo.p_monitor.frequency_var),
        servo.p_monitor.counter
    );      
    ESP_LOGI(TAG, "IMU service frequency:   %.0fHz < %.0fHz < %.0fHz  [var:%.1fHz] (count:%lld)",
        imu.p_monitor.frequency_min,
        imu.p_monitor.frequency_mean,
        imu.p_monitor.frequency_max,
        sqrtf(imu.p_monitor.frequency_var),
        imu.p_monitor.counter
    );  

    host.f_monitor.compute_rates();
    ESP_LOGI(TAG, "HOST RX frame error rates:  CHKS=%.3f%%   SYNT=%.3f%%   TOUT=%.3f%%   TRUNC=%.3f%%   .",
        host.f_monitor.rate[mini_pupper::frame_error_rate_monitor::CHECKSUM_ERROR]*100.0,
        host.f_monitor.rate[mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR]*100.0,
        host.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TIME_OUT_ERROR]*100.0,
        host.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TRUNCATED_ERROR]*100.0
    );
    servo.f_monitor.compute_rates();
    ESP_LOGI(TAG, "SERVO RX frame error rates: CHKS=%.3f%%   SYNT=%.3f%%   TOUT=%.3f%%   TRUNC=%.3f%%   .",
        servo.f_monitor.rate[mini_pupper::frame_error_rate_monitor::CHECKSUM_ERROR]*100.0,
        servo.f_monitor.rate[mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR]*100.0,
        servo.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TIME_OUT_ERROR]*100.0,
        servo.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TRUNCATED_ERROR]*100.0
    );
    imu.f_monitor.compute_rates();
    ESP_LOGI(TAG, "IMU RX frame error rates:   CHKS=%.3f%%   SYNT=%.3f%%   TOUT=%.3f%%   TRUNC=%.3f%%   .",
        imu.f_monitor.rate[mini_pupper::frame_error_rate_monitor::CHECKSUM_ERROR]*100.0,
        imu.f_monitor.rate[mini_pupper::frame_error_rate_monitor::SYNTAX_ERROR]*100.0,
        imu.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TIME_OUT_ERROR]*100.0,
        imu.f_monitor.rate[mini_pupper::frame_error_rate_monitor::TRUNCATED_ERROR]*100.0
    );

    return 0;
}

static void register_mini_pupper_cmd_stats(void)
{
    const esp_console_cmd_t cmd_servo_getLoad = {
        .command = "top",
        .help = "return statistics",
        .hint = "no params",
        .func = &mini_pupper_cmd_stats,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_servo_getLoad) );
}

void register_mini_pupper_cmds(void)
{
    register_mini_pupper_cmd_scan();
    register_mini_pupper_cmd_setID();
    register_mini_pupper_cmd_calibrate_begin();
    register_mini_pupper_cmd_calibrate_end();
    register_mini_pupper_cmd_calibrate_clear();
    register_mini_pupper_cmd_ota();
    // register_mini_pupper_cmd_extended_menu();

    // register_mini_pupper_cmd_stats();
}

void register_mini_pupper_extended_cmds(void)
{
    register_mini_pupper_cmd_disable();
    register_mini_pupper_cmd_enable();
    register_mini_pupper_cmd_isEnabled();
    register_mini_pupper_cmd_disableTorque();
    register_mini_pupper_cmd_enableTorque();
    register_mini_pupper_cmd_isTorqueEnabled();
    register_mini_pupper_cmd_setPosition();
    register_mini_pupper_cmd_setPosition12();
    register_mini_pupper_cmd_getPosition();
    register_mini_pupper_cmd_getSpeed();
    register_mini_pupper_cmd_getLoad();
    register_mini_pupper_cmd_getVoltage();
    register_mini_pupper_cmd_getTemperature();
    register_mini_pupper_cmd_getMove();
    register_mini_pupper_cmd_getCurrent();
    register_mini_pupper_cmd_setPositionAsync();
    register_mini_pupper_cmd_setPosition12Async();
    register_mini_pupper_cmd_getPositionAsync();
    register_mini_pupper_cmd_getSpeedAsync();
    register_mini_pupper_cmd_getLoadAsync();
    register_imu_cmds();
    register_system();
    register_wifi();
}
