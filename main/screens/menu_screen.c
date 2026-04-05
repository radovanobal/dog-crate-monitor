#include <stdbool.h>

#include "esp_log.h"
#include "epaper_port.h"

#include "menu_screen.h"
#include "../screen_manager.h"
#include "../screen_types.h"
#include "../screen_layout.h"
#include "../screen_render.h"
#include "../display_types.h"
#include "../utils/macros.h"

typedef struct {
    char text[16];
    sFONT *font;
    ScreenId targetScreenId;
} MenuItem;

typedef struct {
    MenuItem items[5];
    size_t count;
    int selectedIndex;
} MenuState;

static const char *TAG = "menu_screen";

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 1
};

static DisplayRegionDescriptor displayRegions[] = {
    [DISPLAY_REGION_MAIN_MENU] = { .id = DISPLAY_REGION_MAIN_MENU },

};

static DirtyRegionEntry dirtyDisplayRegions[] = {
    [DISPLAY_REGION_MAIN_MENU] = { .regionId = DISPLAY_REGION_MAIN_MENU, .isDirty = false },
};

static ScreenLayout screenLayout;
static MenuState menuState = {0};

static void initDisplay(void);
static void initRenderRegions(void);
static void initMenuState(void);
static void deinitDisplay(void);
static void determineDirtyRegions(void);
static void markActiveMenuItem(const AppState *state);
static DisplayRenderPlan buildDisplayRenderPlan(const AppState *state);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state);
static ScreenRenderResult evaluateDisplay(const AppState *state);

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
    initRenderRegions();
    initMenuState();
}

static void initRenderRegions(void)
{
    displayRegions[DISPLAY_REGION_MAIN_MENU].gridRegion = (struct GridRegion){ .x = 0, .y = 0, .width = 1, .height = 1 };

    const int regionCount = ARRAY_SIZE(displayRegions);
    calculateDisplayRegionsPixelSpace(displayRegions, regionCount, screenLayout);
}

static void initMenuState(void) {
    menuState = (MenuState){
        .items = {
            [0] = (MenuItem){
                .text = "Home",
                .font = &Font18,
                .targetScreenId = SCREEN_ID_HOME
            }
        },
        .count = 1,
        .selectedIndex = 0
    };
}

static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state) {

    return (ScreenActionResult){
        .screenIntent = {
            .intentType = SCREEN_INTENT_TYPE_NONE
        }
    };
}

static void markActiveMenuItem(const AppState *state) {
    for (size_t i = 0; i < menuState.count; i++) {
        if(menuState.items[i].targetScreenId == state->sharedState.navigationState.activeScreen) {
            menuState.selectedIndex = i;
            break;
        }
    }
}

static ScreenRenderResult evaluateDisplay(const AppState *state) {
    DisplayRenderPlan displayRenderPlan = buildDisplayRenderPlan(state);

    markActiveMenuItem(state);

    return (ScreenRenderResult){
        .displayRenderPlan = displayRenderPlan
    };
}

static DisplayRenderPlan buildDisplayRenderPlan(const AppState *state) {
    DisplayRenderPlan displayRenderPlan = {0};
    size_t renderItemIndex = 0;

    determineDirtyRegions();

    if (dirtyDisplayRegions[DISPLAY_REGION_MAIN_MENU].isDirty) {
        struct PixelCoordinates2D textPosition = calculateAlignedTextPosition(
            &displayRegions[DISPLAY_REGION_MAIN_MENU], 
            menuState.items[menuState.selectedIndex].text, 
            menuState.items[menuState.selectedIndex].font, 
            REGION_ALIGNMENT_TOP_LEFT
        );

        PixelRenderItem menuItemRenderItem = createTextRenderItem(textPosition, menuState.items[menuState.selectedIndex].text, menuState.items[menuState.selectedIndex].font);
        displayRenderPlan.regions[renderItemIndex++].renderItems[0] = menuItemRenderItem;
    }

    displayRenderPlan.count = renderItemIndex;
    return displayRenderPlan;
}

static void determineDirtyRegions(void) {
    for (size_t i = 0; i < ARRAY_SIZE(dirtyDisplayRegions); i++) {
        dirtyDisplayRegions[i].isDirty = false;
    }

    //if () {
        dirtyDisplayRegions[DISPLAY_REGION_MAIN_MENU].isDirty = true; // TODO scroll will have to mark this region dirty after first paint
    //}
}

static void deinitDisplay(void) {

}