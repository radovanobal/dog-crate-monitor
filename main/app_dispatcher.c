#include "./app_dispatcher.h"
#include "./app_store.h"

AppState appState;

void initAppDispatcher() {
    appStore_initAppState(&appState);
}

void appDispatcher_dispatchEvent(const AppEvent *event) {
    switch (event->eventType) {
        case APP_EVENT_ENVIRONMENT_UPDATED:
            appDispatcher_handleEnvironmentUpdateEvent(
                event->data.environmentUpdateData.temperatureC, 
                event->data.environmentUpdateData.relativeHumidity, 
                event->data.environmentUpdateData.currentTime
            );
            break;
        case APP_EVENT_INPUT_RECEIVED:
            appDispatcher_handleInputEvent();
            break;    
    }
}


void appDispatcher_handleEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, TimeDate currentTime) {
    appStore_updateEnvironmentState(&appState, temperatureC, relativeHumidity, currentTime);
}

void appDispatcher_handleInputEvent() {
    // Handle input events and update app state as needed
}

const AppState *appDispatcher_getAppState() {
    return &appState;
}