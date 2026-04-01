#include "esp_log.h"

#include "./screen_manager.h"
#include "./screen_types.h"
#include "./display_types.h"
#include "./display_controller.h"
#include "./app_event.h"
#include "./app_store.h"
#include "./screens/home_screen.h"
#include "./screens/menu_screen.h"

static void handleInputEvent(const AppEvent *event, const AppState *state, ScreenActionResult *result);
static ScreenRegistration createScreenRegistration(ScreenId id, const ScreenInterface *interface);
static void ensureActiveScreenRegistered(const AppState *state);

static const char *TAG = "screen_manager";

static ScreenRegistration registeredScreen = {0};
static ScreenGeneration lastScreenGeneration = 0;

void screenManager_render(DisplayRequest *displayRequest) {
    if (displayRequest->screenGeneration != lastScreenGeneration) {  
        ESP_LOGW(TAG, "Attempted to render screen ID %d, but active screen ID is %d. Ignoring render request.",
            displayRequest->screenId, registeredScreen.id
        );
        return;
    }

    displayController_requestRender(&displayRequest->displayRenderPlan, displayRequest->screenGeneration);
}

DisplayRequest screenManager_buildDisplayRequest(ScreenId screenId, ScreenGeneration screenGeneration, const ScreenRenderResult *renderResult) {    
    DisplayRequest displayRequest = {
        .screenId = screenId,
        .screenGeneration = screenGeneration,
        .displayRenderPlan = renderResult->displayRenderPlan,
    };

    lastScreenGeneration = screenGeneration;

    return displayRequest;
}

ScreenActionResult screenManager_handleEvent(const AppEvent *event, const AppState *appState) {
    if (event->eventType == APP_EVENT_INPUT_RECEIVED) {
        ScreenActionResult result = {.screenIntent = { .intentType = SCREEN_INTENT_TYPE_NONE }};
        handleInputEvent(event, appState, &result);
        
        if (result.screenIntent.intentType != SCREEN_INTENT_TYPE_NONE) {
            return result;
        }
    }

    ensureActiveScreenRegistered(appState);
    
    ScreenActionResult screenResult = registeredScreen.interface->handleEvent(event, appState);
    return screenResult;
}

ScreenRenderResult screenManager_evaluateDisplay(const AppState *appState) {
    ensureActiveScreenRegistered(appState);

    return registeredScreen.interface->evaluateDisplay(appState);
}

static void ensureActiveScreenRegistered(const AppState *state) {
    ScreenId activeScreenId = state->sharedState.navigationState.activeScreen;
    const ScreenInterface *activeScreenInterface = registeredScreen.interface;

    const bool isAlreadyRegistered =
        registeredScreen.interface != NULL &&
        registeredScreen.id == activeScreenId &&
        registeredScreen.isInitialized;

    if (isAlreadyRegistered) {
        return;
    }

    if (registeredScreen.interface != NULL && registeredScreen.isInitialized) {
        registeredScreen.interface->deinit();
    }

    switch(activeScreenId) {
        case SCREEN_ID_HOME:
            activeScreenInterface = homeScreen_getScreenInterface();
            break;
        case SCREEN_ID_MENU:
            activeScreenInterface = menuScreen_getScreenInterface();
            break;
        default:   
            ESP_LOGW(TAG, "No screen interface found for screen ID: %d! Defaulting to home screen.", activeScreenId);
            activeScreenId = SCREEN_ID_HOME;
            activeScreenInterface = homeScreen_getScreenInterface();
    }

    registeredScreen = createScreenRegistration(
        activeScreenId, 
        activeScreenInterface
    );

    registeredScreen.interface->init();
    registeredScreen.isInitialized = true;
}

static void handleInputEvent(const AppEvent *event, const AppState *state, ScreenActionResult *result) {
    if (event->data.inputEventData.buttonType == BUTTON_EVENT_TYPE_BUTTON_SELECT) {
        ScreenIntent intent = {
            .intentType = SCREEN_INTENT_SET_ACTIVE_SCREEN,
            .data.screenId = (state->sharedState.navigationState.activeScreen + 1) % 2 // Example: toggle between two screens
        };
        result->screenIntent = intent;
    }
}

static ScreenRegistration createScreenRegistration(ScreenId id, const ScreenInterface *interface) {
    return (ScreenRegistration){
        .id = id,
        .isInitialized = false,
        .interface = interface
    };
}