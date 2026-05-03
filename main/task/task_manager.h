#ifndef DOG_CRATE_MONITOR_TASK_MANAGER_H
#define DOG_CRATE_MONITOR_TASK_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    TASK_MANAGER_SUCCESS = 0,
    TASK_MANAGER_FAIL = -1
} task_manager_error;

task_manager_error initTaskManager(void);
void uiTask(void *pvParameters);
void renderTask(void *pvParameters);
void inputTask(void *pvParameters);
void serviceTask(void *pvParameters);

#endif //DOG_CRATE_MONITOR_TASK_MANAGER_H