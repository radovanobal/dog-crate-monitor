#ifndef DOG_CRATE_MONITOR_DISPLAY_H
#define DOG_CRATE_MONITOR_DISPLAY_H

#include "./display_types.h"
#include "./environment_types.h"
#include "./app_event.h"
#include "./app_store.h"

enum display_error {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
};

typedef struct {
    bool isRenderRequired;
    DisplayState displayState;
} HomeScreenResult;

enum display_error initDisplay(void);
void renderToDisplay(DisplayState *displayState);
void partialRenderToDisplay(DisplayState *displayState);
void homeScreen_setLastEnqueuedDisplayState(const DisplayState *displayState);
HomeScreenResult homeScreen_handleEvent(const AppEvent *event, const AppState *appState);

#endif // DOG_CRATE_MONITOR_DISPLAY_H
