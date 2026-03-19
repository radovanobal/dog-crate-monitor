#ifndef DOG_CRATE_MONITOR_TASK_MANAGER_H
#define DOG_CRATE_MONITOR_TASK_MANAGER_H

#include "display_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "./environment_types.h"

typedef enum {
    TASK_MANAGER_SUCCESS = 0,
    TASK_MANAGER_FAIL = -1
} task_manager_error;


task_manager_error initTaskManager(void);
void uiTask(void *pvParameters);
void renderTask(void *pvParameters);
void serviceTask(void *pvParameters);

typedef enum {
    DISPLAY_PAINT_TYPE_FULL,
    DISPLAY_PAINT_TYPE_PARTIAL
} DisplayPaintType;

typedef enum {
    APP_EVENT_ENVIRONMENT_UPDATED,
} AppEventType;

typedef struct {
    DisplayPaintType paintType;
//    DisplayRegionId regionId;
    DisplayState displayState;
} DisplayRequest;

typedef struct {
    AppEventType eventType;
    struct {
        float temperatureC;
        float relativeHumidity;
    } sensorUpdateData;
    TimeDate timeUpdateData;
} AppEvent;

#endif //DOG_CRATE_MONITOR_TASK_MANAGER_H