#ifndef DOG_CRATE_MONITOR_APP_EVENT_H
#define DOG_CRATE_MONITOR_APP_EVENT_H

#include "./environment_types.h"

typedef enum {
    APP_EVENT_ENVIRONMENT_UPDATED = 0,
    APP_EVENT_INPUT_RECEIVED = 1
} AppEventType;

typedef enum {
    INPUT_ACTION_ROTARY_ENCODER_UP = 0,
    INPUT_ACTION_ROTARY_ENCODER_DOWN = 1,
    INPUT_ACTION_ROTARY_ENCODER_PRESS = 2,
    INPUT_ACTION_BUTTON_BACK = 3, // Hardware button label: pwr
    INPUT_ACTION_BUTTON_SELECT = 4, // Hardware button label: boot
} InputAction;

typedef struct {
    float temperatureC;
    float relativeHumidity;
    TimeDate currentTime;
} EnvironmentUpdateData;

typedef struct {
    InputAction action;
} InputEventData;

typedef struct {
    AppEventType eventType;
    union {
        EnvironmentUpdateData sensorUpdateData;
        InputEventData inputEventData;
    } data;
} AppEvent;

AppEvent appEvent_createEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, TimeDate currentTime);
AppEvent appEvent_createInputEvent(InputAction action);

#endif // DOG_CRATE_MONITOR_APP_EVENT_H