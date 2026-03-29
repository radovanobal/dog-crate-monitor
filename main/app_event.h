#ifndef DOG_CRATE_MONITOR_APP_EVENT_H
#define DOG_CRATE_MONITOR_APP_EVENT_H

#include "./environment_types.h"
#include "./button_event.h"

typedef enum {
    APP_EVENT_ENVIRONMENT_UPDATED = 0,
    APP_EVENT_INPUT_RECEIVED = 1
} AppEventType;

typedef struct {
    float temperatureC;
    float relativeHumidity;
    TimeDate currentTime;
} EnvironmentUpdateData;

typedef struct {
    ButtonType buttonType;
    ButtonPressType pressType;
} InputEventData;

typedef struct {
    AppEventType eventType;
    union {
        EnvironmentUpdateData environmentUpdateData;
        InputEventData inputEventData;
    } data;
} AppEvent;

AppEvent appEvent_createEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, TimeDate currentTime);
AppEvent appEvent_createInputEvent(ButtonType buttonType, ButtonPressType pressType);

#endif // DOG_CRATE_MONITOR_APP_EVENT_H