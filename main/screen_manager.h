#ifndef DOG_CRATE_MONITOR_SCREEN_MANAGER_H
#define DOG_CRATE_MONITOR_SCREEN_MANAGER_H

#include <stdbool.h>

#include "./app_event.h"
#include "./app_store.h"
#include "./display_types.h"
#include "./screen_types.h"

typedef enum {
    SCREEN_INTENT_TYPE_NONE = 0,
    SCREEN_INTENT_SET_ACTIVE_SCREEN = 1,
} ScreenIntentType;

typedef struct {
    ScreenIntentType intentType;
    union {
        int screenId; // For SCREEN_INTENT_SET_ACTIVE_SCREEN
    } data;
} ScreenIntent;

typedef struct {
    ScreenIntent screenIntent;
} ScreenActionResult;

typedef struct {
    bool isRenderRequired;
    DisplayState displayState;
} ScreenRenderResult;

typedef struct {
    enum display_error (*init)(void);
    void (*deinit)(void);
    void (*fullRenderToDisplay)(DisplayState *displayState);
    void (*partialRenderToDisplay)(DisplayState *displayState);
    void (*setLastEnqueuedDisplayState)(const DisplayState *displayState);
    ScreenActionResult (*handleEvent)(const AppEvent *event, const AppState *appState);
    ScreenRenderResult (*evaluateDisplay)(const AppState *appState);
} ScreenInterface;

typedef struct {
    ScreenId id;
    bool isInitialized;
    const ScreenInterface *interface;
} ScreenRegistration;

ScreenActionResult screenManager_handleEvent(const AppEvent *event, const AppState *appState);
ScreenRenderResult screenManager_evaluateDisplay(const AppState *appState);
DisplayRequest screenManager_buildDisplayRequest(ScreenId screenId, const DisplayState *currentDisplayState);
void screenManager_render(DisplayRequest *displayRequest);
void screenManager_setLastEnqueuedDisplayState(const DisplayRequest *displayRequest);

#endif //DOG_CRATE_MONITOR_SCREEN_MANAGER_H