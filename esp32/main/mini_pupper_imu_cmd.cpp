#include "mini_pupper_imu_cmd.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "mini_pupper_imu.h"
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "argtable3/argtable3.h"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "IMUCMD";

static uint64_t end_time = 0;

static int imu_cmd_init(int argc, char **argv)
{
    uint8_t err;
    err = imu.init();
    if(err) {
        printf("Init error: %d \r\n", err);
    }
    return 0;
}

static struct {
    struct arg_int *loop;
    struct arg_end *end;
} imu_loop_args;

static void register_imu_cmd_init(void)
{
    const esp_console_cmd_t cmd_imu_init = {
        .command = "imu-init",
        .help = "init the imu",
        .hint = NULL,
        .func = &imu_cmd_init,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_imu_init) );
}

static int imu_cmd_who_am_i(int argc, char **argv)
{
    uint8_t ret;
    ret = imu.who_am_i();
    printf("who_am_i: %d \r\n", ret);
    return 0;
}

static void register_imu_cmd_who_am_i(void)
{
    const esp_console_cmd_t cmd_imu_who_am_i = {
        .command = "imu-who_am_i",
        .help = "return who_am_i from the imu",
        .hint = NULL,
        .func = &imu_cmd_who_am_i,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_imu_who_am_i) );
}

static int imu_cmd_revision(int argc, char **argv)
{
    uint8_t ret;
    ret = imu.revision();
    printf("revision: %d \r\n", ret);
    return 0;
}

static void register_imu_cmd_revision(void)
{
    const esp_console_cmd_t cmd_imu_version = {
        .command = "imu-version",
        .help = "return version from the imu",
        .hint = NULL,
        .func = &imu_cmd_revision,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_imu_version) );
}

static int imu_cmd_read_6dof(int argc, char **argv)
{
    uint8_t err;

    int nerrors = arg_parse(argc, argv, (void **)&imu_loop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, imu_loop_args.end, argv[0]);
        return 0;
    }

    int loop = imu_loop_args.loop->ival[0];

    for(int i=0; i<loop; i++) {
        err = imu.read_6dof();
        if(err) {
            printf("error: %d \r\n", err);
        }
        else {
            printf("Ax:%.3f Ay:%.3f Az:%.3f | Gx:%.3f Gy:%.3f Gz:%.3f. ", imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz);
        }
	vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    return 0;
}

static void register_imu_cmd_read_6dof(void)
{
    imu_loop_args.loop = arg_int1(NULL, "loop", "<n>", "loop <n> times");
    imu_loop_args.end = arg_end(2);
    const esp_console_cmd_t cmd_imu_read_6dof = {
        .command = "imu-read_6dof",
        .help = "read 6dof the imu",
        .hint = "--loop <n>",
        .func = &imu_cmd_read_6dof,
	.argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_imu_read_6dof) );
}

void register_imu_cmds(void)
{
    register_imu_cmd_init();
    register_imu_cmd_who_am_i();
    register_imu_cmd_revision();
    register_imu_cmd_read_6dof();
}
