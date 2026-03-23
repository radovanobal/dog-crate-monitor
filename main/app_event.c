#include "./app_event.h"


AppEvent appEvent_createEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, TimeDate currentTime) {
    AppEvent event = {
        .eventType = APP_EVENT_ENVIRONMENT_UPDATED,
        .data.sensorUpdateData = {
            .temperatureC = temperatureC,
            .relativeHumidity = relativeHumidity,
            .currentTime = currentTime
        }
    };
    return event;
}

AppEvent appEvent_createInputEvent(InputAction action) {
    AppEvent event = {
        .eventType = APP_EVENT_INPUT_RECEIVED,
        .data.inputEventData = {
            .action = action
        }
    };
    return event;
}