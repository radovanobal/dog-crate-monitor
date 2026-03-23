
#include "./app_store.h"
#include "./environment_types.h"

static void initEnvironment(AppState *appState);
static void initScreenState(AppState *appState);

void appStore_initAppState(AppState *appState) {
    initEnvironment(appState);
    initScreenState(appState);
}

static void initEnvironment(AppState *appState) {
    appState->sharedState.environmentState.temperatureC = 0.0f;
    appState->sharedState.environmentState.relativeHumidity = 0.0f;
    appState->sharedState.environmentState.currentTime = (TimeDate){0};
}

static void initScreenState(AppState *appState) {
    appState->sharedState.navigationState.activeScreen = SCREEN_ID_HOME;
}

void appStore_updateEnvironmentState(AppState *appState, float temperatureC, float relativeHumidity, TimeDate currentTime) {
    appState->sharedState.environmentState.temperatureC = temperatureC;
    appState->sharedState.environmentState.relativeHumidity = relativeHumidity;
    appState->sharedState.environmentState.currentTime = currentTime;
}

void appStore_updateNavigationState(AppState *appState, ScreenId activeScreen) {
    appState->sharedState.navigationState.activeScreen = activeScreen;
}

