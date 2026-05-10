#include "display/display_types.h"
#include "esp_log.h"

#include "app/app_event.h"
#include "app/app_store.h"
#include "display/display_controller.h"
#include "screen/screen_manager.h"
#include "screen/screen_types.h"
#include "screens/home_screen.h"
#include "screens/menu_screen.h"
#include "screens/settings_screen.h"
#include "utils/macros.h"

static void handleInputEvent(const AppEvent *event, const AppState *state, ScreenActionResult *result);
static ScreenRegistration createScreenRegistration(ScreenId id, const ScreenInterface *interface);
static void ensureActiveScreenRegistered(const AppState *state);
static bool isNonReturnablePurpose(ScreenPurpose purpose);
static DisplayPrepareRequest evaluateDisplay(const AppState *appState, DisplayRenderPlan *renderPlan, DisplayPipelineType *pipelineType);

static const char *TAG = "screen_manager";

static ScreenRegistration registeredScreen = {0};
static ScreenGeneration lastScreenGeneration = 0;
static const ScreenPurpose nonReturnablePurposes[] = { SCREEN_PURPOSE_NAVIGATION };
static ScreenId lastDataScreenId = SCREEN_ID_HOME;

void screenManager_render(DisplayRequest *displayRequest) {
    if (displayRequest->screenGeneration != lastScreenGeneration) {  
        ESP_LOGW(TAG, "Attempted to render screen ID %d, but active screen ID is %d. Ignoring render request.",
            displayRequest->screenId, registeredScreen.id
        );
        return;
    }

    displayController_requestRender(displayRequest);
}

DisplayPrepareRequest screenManager_buildDisplayRequest(const AppState *state, ScreenId screenId, ScreenGeneration screenGeneration, DisplayRequest *displayRequest) {    
    *displayRequest = (DisplayRequest){
        .screenId = screenId,
        .screenGeneration = screenGeneration,
        .displayRenderPlan = { .count = 0 },
        .pipelineType = DISPLAY_PIPELINE_TYPE_MONO
    };

    DisplayPrepareRequest prepareResult = evaluateDisplay(state, &displayRequest->displayRenderPlan, &displayRequest->pipelineType);

    lastScreenGeneration = screenGeneration;

    return prepareResult;
}

ScreenActionResult screenManager_handleEvent(const AppEvent *event, const AppState *appState) {
    if (event->eventType == APP_EVENT_INPUT_RECEIVED) {
        ScreenActionResult result = {
            .screenIntent = { .intentType = SCREEN_INTENT_TYPE_NONE }
        };
        handleInputEvent(event, appState, &result);
        
        if (result.screenIntent.intentType != SCREEN_INTENT_TYPE_NONE) {
            return result;
        }
    }

    ensureActiveScreenRegistered(appState);
    
    ScreenActionResult screenResult = registeredScreen.interface->handleEvent(event, appState);
    return screenResult;
}

static DisplayPrepareRequest evaluateDisplay(const AppState *appState, DisplayRenderPlan *renderPlan, DisplayPipelineType *pipelineType) {
    ensureActiveScreenRegistered(appState);

    return registeredScreen.interface->evaluateDisplay(appState, renderPlan, pipelineType);
}

static bool tryGetScreenInterface(ScreenId screenId, const ScreenInterface **outInterface) {
    switch (screenId) {
        case SCREEN_ID_HOME:
            *outInterface = homeScreen_getScreenInterface();
            return true;
        case SCREEN_ID_MENU:
            *outInterface = menuScreen_getScreenInterface();
            return true;
        case SCREEN_ID_SETTINGS:
            *outInterface = settingsScreen_getScreenInterface();
            return true;
        default:
            ESP_LOGW(TAG, "No screen interface found for screen ID: %d", screenId);
            return false;
    }

    return false;
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

    if (!tryGetScreenInterface(activeScreenId, &activeScreenInterface)) {
        ESP_LOGE(TAG, "Failed to get screen interface for active screen ID: %d. Defaulting to home screen.", activeScreenId);
        activeScreenId = SCREEN_ID_HOME;
        tryGetScreenInterface(activeScreenId, &activeScreenInterface);
    }

    registeredScreen = createScreenRegistration(
        activeScreenId, 
        activeScreenInterface
    );

    registeredScreen.interface->init(state);
    registeredScreen.isInitialized = true;

    if(isNonReturnablePurpose(registeredScreen.interface->purpose)) {
        lastDataScreenId = activeScreenId;
    }
}

static bool isNonReturnablePurpose(ScreenPurpose purpose) {
    for (size_t i = 0; i < ARRAY_SIZE(nonReturnablePurposes); i++) {
        if (nonReturnablePurposes[i] == purpose) {
            return false;
        }
    }

    return true;
}

static void handleInputEvent(const AppEvent *event, const AppState *state, ScreenActionResult *result) {
    if (event->data.inputEventData.buttonType == BUTTON_EVENT_TYPE_BUTTON_SELECT) {
        const ScreenId currentActiveScreenId = state->sharedState.navigationState.activeScreen == SCREEN_ID_MENU ? lastDataScreenId : SCREEN_ID_MENU;
        const bool isMenuNavigation = isNonReturnablePurpose(registeredScreen.interface->purpose);

        ScreenIntent intent = {
            .intentType = SCREEN_INTENT_TYPE_SCREEN_CHANGE,
            .data = {
                .screenId = currentActiveScreenId,
                .isMenuNavigation = isMenuNavigation
            }
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