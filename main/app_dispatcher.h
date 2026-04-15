#ifndef DOG_CRATE_MONITOR_APP_DISPATCHER_H
#define DOG_CRATE_MONITOR_APP_DISPATCHER_H

#include "./environment_types.h"
#include "./app_event.h"
#include "./app_store.h"
#include "./screen_manager.h"

void appDispatcher_init();
void appDispatcher_dispatchEvent(const AppEvent *event);
void appDispatcher_applyScreenIntent(const ScreenIntent *intent);
void appDispatcher_handleEnvironmentUpdateEvent(float temperatureC, float relativeHumidity, int batteryLevel, TimeDate currentTime);
const AppState *appDispatcher_getAppState();

#endif // DOG_CRATE_MONITOR_APP_DISPATCHER_H