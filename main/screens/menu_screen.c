#include <stdbool.h>

#include "epaper_port.h"

#include "menu_screen.h"
#include "../screen_manager.h"
#include "../screen_types.h"
#include "../screen_layout.h"
#include "../display_types.h"
#include "../utils/macros.h"
#include "../widgets/list_navigation.h"

static const char *TAG = "menu_screen";

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 1
};

static const DisplayRegionId DISPLAY_REGION_MAIN_MENU = 0;

enum {
    MENU_REGION_SLOT_MAIN_MENU = 0,
};

static DisplayRegionDescriptor displayRegions[] = {
    [MENU_REGION_SLOT_MAIN_MENU] = { .id = DISPLAY_REGION_MAIN_MENU },

};

static ScreenLayout screenLayout;
static MenuState menuState = {0};

static void initDisplay(const AppState *state);
static void initRenderRegions(void);
static void initMenuState(const AppState *state);
static void deinitDisplay(void);
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

static void initDisplay(const AppState *state) {
    screenLayout = initRenderGrid(gridConfig);
    initRenderRegions();
    initMenuState(state);
}

static void initRenderRegions(void)
{
    displayRegions[MENU_REGION_SLOT_MAIN_MENU].gridRegion = (struct GridRegion){ .x = 0, .y = 0, .width = 1, .height = 1 };

    const int regionCount = ARRAY_SIZE(displayRegions);
    calculateDisplayRegionsPixelSpace(displayRegions, regionCount, screenLayout);
}

static void initMenuState(const AppState *state) {
    menuState = (MenuState){
        .items = {
            [0] = (MenuItem){
                .text = "Home",
                .font = &Font18,
                .targetScreenId = SCREEN_ID_HOME
            },
            [1] = (MenuItem){
                .text = "Settings",
                .font = &Font18,
                .targetScreenId = SCREEN_ID_SETTINGS
            },
        },
        .count = 2,
        .activeId = state->sharedState.navigationState.activeScreen
    };

    const ListNavigationItem listItems[] = {
        {
            .label = menuState.items[0].text,
            .font = menuState.items[0].font,
            .id = menuState.items[0].targetScreenId,
            .isEnabled = true
        },
        {
            .label = menuState.items[1].text,
            .font = menuState.items[1].font,
            .id = menuState.items[1].targetScreenId,
            .isEnabled = true
        }
    };


    listNavigation_init(listItems, ARRAY_SIZE(listItems), menuState.activeId);
}

static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state) {
    ScreenActionResult intent = {
        .screenIntent = {
            .intentType = SCREEN_INTENT_TYPE_NONE
        }
    };

    if (event->eventType != APP_EVENT_INPUT_RECEIVED) {
        return intent;
    }

    ListNavigationResult navResult = listNavigation_handleInput(event->data.inputEventData);

    if(navResult.actionType == LIST_NAVIGATION_ACTION_SELECTION_CONFIRMED) {
        ScreenId targetScreenId = navResult.selectedItemId;
        intent.screenIntent.intentType = SCREEN_INTENT_TYPE_SCREEN_CHANGE;
        intent.screenIntent.data.screenId = targetScreenId;
    }

    return intent;
}


static void markActiveMenuItem(const AppState *state) {
    for (size_t i = 0; i < menuState.count; i++) {
        if(menuState.items[i].targetScreenId == state->sharedState.navigationState.activeScreen) {
            menuState.activeId = menuState.items[i].targetScreenId;
            break;
        }
    }
}

static ScreenRenderResult evaluateDisplay(const AppState *state) {
    markActiveMenuItem(state);
    listNavigation_setActiveItem(state->sharedState.navigationState.lastNonMenuScreen);
    DisplayRenderPlan displayRenderPlan = buildDisplayRenderPlan(state);

    return (ScreenRenderResult){
        .displayRenderPlan = displayRenderPlan
    };
}

static DisplayRenderPlan buildDisplayRenderPlan(const AppState *state) {
    DisplayRenderPlan displayRenderPlan = {0};

    listNavigation_buildRenderPlan(&displayRenderPlan);

    return displayRenderPlan;
}

static void deinitDisplay(void) {
    menuState = (MenuState){0};

    listNavigation_deinit();
}