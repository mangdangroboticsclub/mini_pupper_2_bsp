/* Authors : 
 * - Hdumcke
 * - Pat92fr
 */

#ifndef _mini_pupper_tasks_H
#define _mini_pupper_tasks_H

enum e_mini_pupper_task_priorities
{
    SERVO_PRIORITY  =   8,
    HOST_PRIORITY   =   7,
    IMU_PRIORITY    =   6
};

#define SERVO_CORE 1
#define HOST_CORE 0
#define IMU_CORE 0

#endif //_mini_pupper_tasks_H
