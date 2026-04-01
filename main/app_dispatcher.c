#include "./app_dispatcher.h"
#include "./app_store.h"

AppState appState;

void appDispatcher_init() {
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
             // Currently, input events are handled directly in the screen manager's handleEvent function, so we don't need to do anything here. If there were any global input handling logic that needed to be applied regardless of the active screen, it could be implemented here.
             break;    
        default:
            break;
    }
}


void appDispatcher_handleEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, TimeDate currentTime) {
    appStore_updateEnvironmentState(&appState, temperatureC, relativeHumidity, currentTime);
}

void appDispatcher_applyScreenIntent(const ScreenIntent *intent) {
    if (intent->intentType == SCREEN_INTENT_SET_ACTIVE_SCREEN) {
        appStore_updateNavigationState(&appState, intent->data.screenId);
    }
}

const AppState *appDispatcher_getAppState() {
    return &appState;
}