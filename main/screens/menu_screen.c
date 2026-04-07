#include <stdbool.h>

#include "esp_log.h"
#include "epaper_port.h"

#include "menu_screen.h"
#include "../button_event.h"
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
    int activeIndex;
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
static bool setMenuItemIndicator(size_t index, struct PixelCoordinates2D textPosition, PixelRenderItem *indicator);
static bool setMenuItemIndicator(size_t index, struct PixelCoordinates2D textPosition, PixelRenderItem *indicator);
static DisplayRenderPlan buildDisplayRenderPlan(const AppState *state);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state);
static ScreenRenderResult evaluateDisplay(const AppState *state);
static struct PixelCoordinates2D setMenuItemPosition(size_t index);


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
            },
            [1] = (MenuItem){
                .text = "Settings",
                .font = &Font18,
                .targetScreenId = SCREEN_ID_SETTINGS
            },
        },
        .count = 2,
        .selectedIndex = 0,
        .activeIndex = 0
    };
}

static ScreenActionResult handleEvent(const AppEvent *event, const AppState *state) {
    if (event->eventType == APP_EVENT_INPUT_RECEIVED) {
        if (event->data.inputEventData.buttonType == BUTTON_EVENT_TYPE_ROTARY_ENCODER_UP) {
            menuState.selectedIndex = (menuState.selectedIndex - 1 + menuState.count) % menuState.count;
            ESP_LOGI(TAG, "Menu item up selected. New selected index: %d", menuState.selectedIndex);
        } else if (event->data.inputEventData.buttonType == BUTTON_EVENT_TYPE_ROTARY_ENCODER_DOWN) {
            menuState.selectedIndex = (menuState.selectedIndex + 1) % menuState.count;
            ESP_LOGI(TAG, "Menu item down selected. New selected index: %d", menuState.selectedIndex);
        } else if (event->data.inputEventData.buttonType == BUTTON_EVENT_TYPE_ROTARY_ENCODER_PRESS) {
            ScreenId targetScreenId = menuState.items[menuState.selectedIndex].targetScreenId;
            ScreenIntent intent = {
                .intentType = SCREEN_INTENT_TYPE_SCREEN_CHANGE,
                .data.screenId = targetScreenId
            };
            ESP_LOGI(TAG, "Menu item select pressed. Changing to screen ID: %d", targetScreenId);
            return (ScreenActionResult){
                .screenIntent = intent
            };
        }
    }

    return (ScreenActionResult){
        .screenIntent = {
            .intentType = SCREEN_INTENT_TYPE_NONE
        }
    };
}


static void markActiveMenuItem(const AppState *state) {
    for (size_t i = 0; i < menuState.count; i++) {
        if(menuState.items[i].targetScreenId == state->sharedState.navigationState.activeScreen) {
            menuState.activeIndex = i;
            break;
        }
    }
}

static ScreenRenderResult evaluateDisplay(const AppState *state) {
    markActiveMenuItem(state);
    DisplayRenderPlan displayRenderPlan = buildDisplayRenderPlan(state);

    return (ScreenRenderResult){
        .displayRenderPlan = displayRenderPlan
    };
}

static DisplayRenderPlan buildDisplayRenderPlan(const AppState *state) {
    DisplayRenderPlan displayRenderPlan = {0};

    determineDirtyRegions();

    if (!dirtyDisplayRegions[DISPLAY_REGION_MAIN_MENU].isDirty) {
        return displayRenderPlan;
    }

    RenderRegionScene menuItemScene = {
        .regionId = DISPLAY_REGION_MAIN_MENU,
        .pixelRegion = displayRegions[DISPLAY_REGION_MAIN_MENU].pixelRegion,
        .renderItems = {{0}},
        .count = 0
    };

    for (size_t i = 0; i < menuState.count; i++) {
        struct PixelCoordinates2D textPosition = setMenuItemPosition(i);

        PixelRenderItem menuItemRenderItem = createTextRenderItem(textPosition, menuState.items[i].text, menuState.items[i].font);
        menuItemScene.renderItems[menuItemScene.count++] = menuItemRenderItem;

        PixelRenderItem indicatorRenderItem = {0};
        if (setMenuItemIndicator(i, textPosition, &indicatorRenderItem)) {
            menuItemScene.renderItems[menuItemScene.count++] = indicatorRenderItem;
        }
    }
    
    displayRenderPlan.regions[displayRenderPlan.count++] = menuItemScene;
    return displayRenderPlan;
}

static bool setMenuItemIndicator(size_t index, struct PixelCoordinates2D textPosition, PixelRenderItem *indicator) {
    if (index == menuState.selectedIndex) {
        *indicator = createTextUnderlineRenderItem(
            textPosition, 
            menuState.items[index].text, 
            menuState.items[index].font, 
            DOT_PIXEL_2X2
        );

        return true;
    }

    if (index == menuState.activeIndex) {
        *indicator = createTextUnderlineRenderItem(
            textPosition, 
            menuState.items[index].text, 
            menuState.items[index].font, 
            DOT_PIXEL_1X1
        );

        return true;
    }

    return false;
}

static struct PixelCoordinates2D setMenuItemPosition(size_t index) {
    struct PixelCoordinates2D position = calculateAlignedTextPosition(
        &displayRegions[DISPLAY_REGION_MAIN_MENU], 
        menuState.items[index].text, 
        menuState.items[index].font, 
        REGION_ALIGNMENT_TOP_LEFT
    );

    position.y += index * (menuState.items[index].font->Height + 10); // Add vertical spacing between items

    return position;
}

static void determineDirtyRegions(void) {
    for (size_t i = 0; i < ARRAY_SIZE(dirtyDisplayRegions); i++) {
        dirtyDisplayRegions[i].isDirty = false;
    }

    dirtyDisplayRegions[DISPLAY_REGION_MAIN_MENU].isDirty = true; // TODO scroll will have to mark this region dirty after first paint
}

static void deinitDisplay(void) {
    menuState = (MenuState){0};
}