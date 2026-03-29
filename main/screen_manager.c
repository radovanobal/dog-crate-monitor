#include "esp_log.h"

#include "./screen_manager.h"
#include "./screen_types.h"
#include "./display_types.h"
#include "./app_event.h"
#include "./app_store.h"
#include "./screens/home_screen.h"

static void handleInputEvent(const AppEvent *event, const AppState *state, ScreenActionResult *result);
static ScreenRegistration createScreenRegistration(ScreenId id, const ScreenInterface *interface);
static void ensureActiveScreenRegistered(const AppState *state);

static const char *TAG = "screen_manager";

static const int maxPartialRenderCount = 100;
static ScreenRegistration registeredScreen = {0};
static int partialRenderCount = maxPartialRenderCount;

void screenManager_render(DisplayRequest *displayRequest) {
    if (displayRequest->screenId != registeredScreen.id) {  
        ESP_LOGW(TAG, "Attempted to render screen ID %d, but active screen ID is %d. Ignoring render request.",
            displayRequest->screenId, registeredScreen.id
        );
        return;
    }

    switch (displayRequest->paintType) {
        case DISPLAY_PAINT_TYPE_FULL:
            registeredScreen.interface->fullRenderToDisplay(&displayRequest->displayState);
            break;
        case DISPLAY_PAINT_TYPE_PARTIAL:
            registeredScreen.interface->partialRenderToDisplay(&displayRequest->displayState);
            break;
        default:
            ESP_LOGW(TAG, "Unknown paint type: %d", displayRequest->paintType);
    }
}

DisplayRequest screenManager_buildDisplayRequest(ScreenId screenId, const DisplayState *displayState) {
    bool isFullRenderRequired = partialRenderCount >= maxPartialRenderCount;
    DisplayPaintType paintType = DISPLAY_PAINT_TYPE_PARTIAL;

    if (isFullRenderRequired) {
        partialRenderCount = 0;
        paintType = DISPLAY_PAINT_TYPE_FULL;
    }
    
    DisplayRequest displayRequest = {
        .paintType = paintType,
        .displayState = *displayState,
        .screenId = screenId
    };

    if (!isFullRenderRequired) {
        partialRenderCount++;
    }

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

void screenManager_setLastEnqueuedDisplayState(const DisplayRequest *displayRequest) {
    if (registeredScreen.interface == NULL ||
        !registeredScreen.isInitialized ||
        registeredScreen.id != displayRequest->screenId) {
        return;
    }

    if (registeredScreen.interface->setLastEnqueuedDisplayState != NULL) {
        registeredScreen.interface->setLastEnqueuedDisplayState(&displayRequest->displayState);
    }
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
            // activeScreenInterface = menuScreen_getScreenInterface();
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

    if (registeredScreen.interface->init() != DISPLAY_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize screen %d", activeScreenId);
        registeredScreen.isInitialized = false;
        return;
    }

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