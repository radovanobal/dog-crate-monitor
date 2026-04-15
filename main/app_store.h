#ifndef DOG_CRATE_MONITOR_APP_STORE_H
#define DOG_CRATE_MONITOR_APP_STORE_H

#include "./environment_types.h"
#include "./screen_types.h"

typedef struct {
    TimeDate currentTime;
    float temperatureC;
    float relativeHumidity;
    int batteryLevel;
} EnvironmentState;

typedef struct {
    ScreenId activeScreen;
    ScreenGeneration screenGeneration;
    ScreenId lastNonMenuScreen;
} NavigationState;

typedef struct {
    NavigationState navigationState;
    EnvironmentState environmentState;
} SharedState;

typedef struct {
    SharedState sharedState;
} AppState;

void appStore_initAppState(AppState *appState);
void appStore_updateEnvironmentState(
    AppState *appState, 
    float temperatureC, 
    float relativeHumidity, 
    int batteryLevel, 
    TimeDate currentTime
);
void appStore_updateNavigationState(AppState *appState, ScreenId activeScreen);

#endif // DOG_CRATE_MONITOR_APP_STORE_H