#ifndef DOG_CRATE_MONITOR_APP_STORE_H
#define DOG_CRATE_MONITOR_APP_STORE_H

#include "./environment_types.h"

typedef struct {
    TimeDate currentTime;
    float temperatureC;
    float relativeHumidity;
} EnvironmentState;

typedef enum {
    SCREEN_ID_HOME = 0,
    SCREEN_ID_MENU = 1
} ScreenId;

typedef struct {
    ScreenId activeScreen;
} NavigationState;

typedef struct {
    NavigationState navigationState;
    EnvironmentState environmentState;
} SharedState;

typedef struct {
    SharedState sharedState;
} AppState;

void appStore_initAppState(AppState *appState);
void appStore_updateEnvironmentState(AppState *appState, float temperatureC, float relativeHumidity, TimeDate currentTime);
void appStore_updateNavigationState(AppState *appState, ScreenId activeScreen);

#endif // DOG_CRATE_MONITOR_APP_STORE_H