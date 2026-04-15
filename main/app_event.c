#include "./app_event.h"


AppEvent appEvent_createEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, int batteryLevel, TimeDate currentTime) {
    AppEvent event = {
        .eventType = APP_EVENT_ENVIRONMENT_UPDATED,
        .data.environmentUpdateData = {
            .temperatureC = temperatureC,
            .relativeHumidity = relativeHumidity,
            .batteryLevel = batteryLevel,
            .currentTime = currentTime
        }
    };
    return event;
}

AppEvent appEvent_createInputEvent(ButtonType buttonType, ButtonPressType pressType) {
    AppEvent event = {
        .eventType = APP_EVENT_INPUT_RECEIVED,
        .data.inputEventData = {
            .buttonType = buttonType,
            .pressType = pressType
        }
    };
    return event;
}