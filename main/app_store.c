#include "esp_log.h"

#include "./app_store.h"
#include "./environment_types.h"
#include "./screen_types.h"

static void initEnvironment(AppState *appState);
static void initScreenState(AppState *appState);

static const char *TAG = "app_store";

void appStore_initAppState(AppState *appState) {
    initEnvironment(appState);
    initScreenState(appState);
}

static void initEnvironment(AppState *appState) {
    appState->sharedState.environmentState.temperatureC = 0.0f;
    appState->sharedState.environmentState.relativeHumidity = 0.0f;
    appState->sharedState.environmentState.currentTime = (TimeDate){0};
    appState->sharedState.environmentState.batteryLevel = 0.0f;
}

static void initScreenState(AppState *appState) {
    appState->sharedState.navigationState.activeScreen = SCREEN_ID_HOME;
    appState->sharedState.navigationState.screenGeneration = 1;
}

void appStore_updateEnvironmentState(AppState *appState, float temperatureC, float relativeHumidity, int batteryLevel, TimeDate currentTime) {
    appState->sharedState.environmentState.temperatureC = temperatureC;
    appState->sharedState.environmentState.relativeHumidity = relativeHumidity;
    appState->sharedState.environmentState.currentTime = currentTime;
    appState->sharedState.environmentState.batteryLevel = batteryLevel;
}

void appStore_updateNavigationState(AppState *appState, ScreenId activeScreen) {
    if (appState->sharedState.navigationState.activeScreen == activeScreen) {
        ESP_LOGW(TAG, "Attempted to update navigation state to the same active screen ID %d. No update performed.", activeScreen);
        return;
    }

    appState->sharedState.navigationState.activeScreen = activeScreen;
    appState->sharedState.navigationState.screenGeneration++;
}

