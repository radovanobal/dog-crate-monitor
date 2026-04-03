#include <stdbool.h>

#include "esp_log.h"
#include "epaper_port.h"

#include "menu_screen.h"
#include "../screen_manager.h"
#include "../screen_types.h"
#include "../screen_layout.h"
#include "../display_types.h"

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 1
};

static DisplayRegionDescriptor displayRegions[] = {
    [DISPLAY_REGION_MAIN_MENU] = {  .id = DISPLAY_REGION_MAIN_MENU },

};

static DirtyRegionEntry dirtyDisplayRegions[] = {
    [DISPLAY_REGION_MAIN_MENU] = { .regionId = DISPLAY_REGION_MAIN_MENU, .isDirty = false },
};

static ScreenLayout screenLayout;

static void initDisplay(void);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state);
static ScreenRenderResult evaluateDisplay(const AppState *state);
static void deinitDisplay(void);

const ScreenInterface *menuScreen_getScreenInterface(void) {
    static const ScreenInterface screenInterface = {
        .purpose = SCREEN_PURPOSE_NAVIGATION,
        .init = initDisplay,
        .handleEvent = handleEvent,
        .evaluateDisplay = evaluateDisplay,
        .deinit = deinitDisplay,
    };

    return &screenInterface;
}

static void initDisplay(void) {
    screenLayout = initRenderGrid(gridConfig);
}

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