#ifndef DOG_CRATE_MONITOR_SCREEN_MANAGER_H
#define DOG_CRATE_MONITOR_SCREEN_MANAGER_H

#include <stdbool.h>

#include "app/app_event.h"
#include "app/app_store.h"
#include "screen/screen_types.h"

typedef enum {
    SCREEN_PURPOSE_NAVIGATION = 0,
    SCREEN_PURPOSE_SETTINGS = 1,
    SCREEN_PURPOSE_DATA_DISPLAY = 2
} ScreenPurpose;

typedef struct {
    ScreenIntentType intentType;
    struct {
        ScreenId screenId; // For SCREEN_INTENT_TYPE_SCREEN_CHANGE
        bool isMenuNavigation; // For SCREEN_INTENT_TYPE_SCREEN_CHANGE, indicates if the navigation is to/from the menu screen
    } data;
} ScreenIntent;

typedef struct {
    ScreenIntent screenIntent;
} ScreenActionResult;

typedef enum {
    DISPLAY_PREPARE_REQUEST_SKIPPED = 0,
    DISPLAY_PREPARE_REQUEST_READY = 1,
} DisplayPrepareRequest;

typedef struct {
    ScreenPurpose purpose;
    void (*init)(const AppState *appState);
    void (*deinit)(void);
    ScreenActionResult (*handleEvent)(const AppEvent *event, const AppState *appState);
    DisplayPrepareRequest (*evaluateDisplay)(const AppState *appState, DisplayRenderPlan *renderPlan, DisplayPipelineType *pipelineType);
} ScreenInterface;

typedef struct {
    ScreenId id;
    bool isInitialized;
    const ScreenInterface *interface;
} ScreenRegistration;

ScreenActionResult screenManager_handleEvent(const AppEvent *event, const AppState *appState);
DisplayPrepareRequest screenManager_buildDisplayRequest(const AppState *state, ScreenId screenId, ScreenGeneration screenGeneration, DisplayRequest *displayRequest);
void screenManager_render(DisplayRequest *displayRequest);

#endif //DOG_CRATE_MONITOR_SCREEN_MANAGER_H