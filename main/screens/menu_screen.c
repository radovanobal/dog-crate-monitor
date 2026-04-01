#include <stdbool.h>

#include "menu_screen.h"
#include "../screen_manager.h"
#include "../screen_types.h"
#include "../display_types.h"

static void initDisplay(void);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state);
static ScreenRenderResult evaluateDisplay(const AppState *state);
static void deinitDisplay(void);

const ScreenInterface *menuScreen_getScreenInterface(void) {
    static const ScreenInterface screenInterface = {
        .init = initDisplay,
        .handleEvent = handleEvent,
        .evaluateDisplay = evaluateDisplay,
        .deinit = deinitDisplay,
    };

    return &screenInterface;
}

static void initDisplay(void) {}

static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state) {
    return (ScreenActionResult){
        .screenIntent = {
            .intentType = SCREEN_INTENT_TYPE_NONE
        }
    };
}

static ScreenRenderResult evaluateDisplay(const AppState *state) {
    return (ScreenRenderResult){
        .displayRenderPlan = {
            .count = 0
        }
    };
}

static void deinitDisplay(void) {

}