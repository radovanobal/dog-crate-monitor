#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "epaper_port.h"

#include "display_types.h"
#include "list_navigation.h"
#include "button_event.h"
#include "font.h"
#include "screen_render.h"
#include "screen_layout.h"
#include "screen_types.h"
#include "app_event.h"

#define MAX_MENU_ITEMS 12
#define MAX_VISIBLE_MENU_ITEMS 6

typedef struct {
    size_t selectedItemIndex;
    size_t firstVisibleIndex;
    ListItemId activeItemId;
    bool hasActiveItem;
    ListNavigationItem items[MAX_MENU_ITEMS];
    size_t itemCount;
} ListNavigationState;

typedef struct {
    size_t selectedItemIndex;
    size_t firstVisibleIndex;
    ListItemId activeItemId;
    bool hasMoreBottomItems;
    bool hasMoreTopItems;
    ListItemId itemSnapshots[MAX_VISIBLE_MENU_ITEMS];
} ListNavigationSnapshot;

typedef uint32_t WidgetRegionId;

const static char *TAG = "list_navigation";

static const ListItemId LIST_ITEM_ID_NONE = UINT16_MAX;

const static WidgetRegionId DISPLAY_REGION_INDICATOR_UP = 0;
const static WidgetRegionId DISPLAY_REGION_ITEM_1 = 1;
const static WidgetRegionId DISPLAY_REGION_ITEM_2 = 2;
const static WidgetRegionId DISPLAY_REGION_ITEM_3 = 3;
const static WidgetRegionId DISPLAY_REGION_ITEM_4 = 4;
const static WidgetRegionId DISPLAY_REGION_ITEM_5 = 5;
const static WidgetRegionId DISPLAY_REGION_ITEM_6 = 6;
const static WidgetRegionId DISPLAY_REGION_INDICATOR_DOWN = 7;

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 8
};

enum {
    DISPLAY_REGION_SLOT_INDICATOR_UP = 0,
    DISPLAY_REGION_SLOT_ITEM_1,
    DISPLAY_REGION_SLOT_ITEM_2,
    DISPLAY_REGION_SLOT_ITEM_3,
    DISPLAY_REGION_SLOT_ITEM_4,
    DISPLAY_REGION_SLOT_ITEM_5,
    DISPLAY_REGION_SLOT_ITEM_6,
    DISPLAY_REGION_SLOT_INDICATOR_DOWN
};

static DisplayRegionDescriptor navigationRegions[] = {
    [DISPLAY_REGION_SLOT_INDICATOR_UP] = { .id = DISPLAY_REGION_INDICATOR_UP },
    [DISPLAY_REGION_SLOT_ITEM_1] = { .id = DISPLAY_REGION_ITEM_1 },
    [DISPLAY_REGION_SLOT_ITEM_2] = { .id = DISPLAY_REGION_ITEM_2 },
    [DISPLAY_REGION_SLOT_ITEM_3] = { .id = DISPLAY_REGION_ITEM_3 },
    [DISPLAY_REGION_SLOT_ITEM_4] = { .id = DISPLAY_REGION_ITEM_4 },
    [DISPLAY_REGION_SLOT_ITEM_5] = { .id = DISPLAY_REGION_ITEM_5 },
    [DISPLAY_REGION_SLOT_ITEM_6] = { .id = DISPLAY_REGION_ITEM_6 },
    [DISPLAY_REGION_SLOT_INDICATOR_DOWN] = { .id = DISPLAY_REGION_INDICATOR_DOWN },
};

static DirtyRegionEntry dirtyNavigationRegions[] = {
    [DISPLAY_REGION_SLOT_INDICATOR_UP] = { .regionId = DISPLAY_REGION_INDICATOR_UP, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_1] = { .regionId = DISPLAY_REGION_ITEM_1, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_2] = { .regionId = DISPLAY_REGION_ITEM_2, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_3] = { .regionId = DISPLAY_REGION_ITEM_3, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_4] = { .regionId = DISPLAY_REGION_ITEM_4, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_5] = { .regionId = DISPLAY_REGION_ITEM_5, .isDirty = false },
    [DISPLAY_REGION_SLOT_ITEM_6] = { .regionId = DISPLAY_REGION_ITEM_6, .isDirty = false },
    [DISPLAY_REGION_SLOT_INDICATOR_DOWN] = { .regionId = DISPLAY_REGION_INDICATOR_DOWN, .isDirty = false },
};

static void updateFirstVisibleIndex(void);
static void setPixelSpace(void);
static void updateMenuSnapshot(void);
static void determineDirtyRegions(void);
static void clearNavigationSlots(DisplayRenderPlan *displayRenderPlan);
static void buildUpIndicatorScene(DisplayRenderPlan *displayRenderPlan);
static void buildDownIndicatorScene(DisplayRenderPlan *displayRenderPlan);
static bool setMenuItemIndicator(size_t index, struct PixelCoordinates2D textPosition, PixelRenderItem *indicator);
static struct PixelCoordinates2D setMenuItemPosition(DisplayRegionDescriptor region, ListNavigationItem item);

static bool isFirstLoad = true;
static ListNavigationState menuState = {0};
static ListNavigationSnapshot previousMenuSnapshot = {0};


void listNavigation_init(const ListNavigationItem *items, size_t itemCount, ListItemId activeItemId){
    menuState.selectedItemIndex = 0;
    menuState.firstVisibleIndex = 0;
    menuState.activeItemId = activeItemId;
    menuState.hasActiveItem = false;
    menuState.itemCount = itemCount < MAX_MENU_ITEMS ? itemCount : MAX_MENU_ITEMS;

    if (itemCount > MAX_MENU_ITEMS) {
        ESP_LOGW("ListNavigation", "Provided item count (%zu) exceeds maximum supported (%d). Truncating list.", itemCount, MAX_MENU_ITEMS);
    }

    for (size_t i = 0; i < menuState.itemCount; i++) {
        menuState.items[i] = items[i];

        if (items[i].id == activeItemId) {
            menuState.selectedItemIndex = i;
            menuState.firstVisibleIndex = (i > 0) ? i - 1 : 0;
            menuState.hasActiveItem = true;
            break;
        }
    }
    
    previousMenuSnapshot = (ListNavigationSnapshot){
        .selectedItemIndex = SIZE_MAX,
        .firstVisibleIndex = SIZE_MAX,
        .activeItemId = LIST_ITEM_ID_NONE,
        .hasMoreBottomItems = false,
        .hasMoreTopItems = false,
    };

    for (size_t i = 0; i < MAX_VISIBLE_MENU_ITEMS; i++) {
        previousMenuSnapshot.itemSnapshots[i] = LIST_ITEM_ID_NONE;
    }

    setPixelSpace();
}

ListNavigationResult listNavigation_handleInput(InputEventData event) {
    ListNavigationResult result = {0};

    if (menuState.itemCount == 0) {
        result.actionType = LIST_NAVIGATION_ACTION_NONE;
        result.selectedItemId = 0;

        ESP_LOGW("ListNavigation", "No items in the list. Ignoring input.");
        return result;
    }

    switch (event.buttonType) {
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_UP:
            menuState.selectedItemIndex = (menuState.selectedItemIndex + menuState.itemCount - 1) % menuState.itemCount;
            result.actionType = LIST_NAVIGATION_ACTION_SELECTION_CHANGED;
            result.selectedItemId = menuState.items[menuState.selectedItemIndex].id;
            break;
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_DOWN:
            menuState.selectedItemIndex = (menuState.selectedItemIndex + 1) % menuState.itemCount;
            result.actionType = LIST_NAVIGATION_ACTION_SELECTION_CHANGED;
            result.selectedItemId = menuState.items[menuState.selectedItemIndex].id;
            break;
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_PRESS:
            result.actionType = LIST_NAVIGATION_ACTION_SELECTION_CONFIRMED;
            result.selectedItemId = menuState.items[menuState.selectedItemIndex].id;
            break;
        default:
            ESP_LOGW("ListNavigation", "Unhandled button event: %d", event.buttonType);
            break;
    }

    updateFirstVisibleIndex();

    return result;
}

void listNavigation_buildRenderPlan(DisplayRenderPlan *displayRenderPlan) {
    if (menuState.itemCount == 0) {
        ESP_LOGW("ListNavigation", "No items to render in the list.");
        clearNavigationSlots(displayRenderPlan);
        return;
    }

    determineDirtyRegions();

    if (dirtyNavigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].isDirty) {
        buildUpIndicatorScene(displayRenderPlan);
    }

    if (dirtyNavigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].isDirty) {
        buildDownIndicatorScene(displayRenderPlan);
    }

    for (size_t slotIndex = 1; slotIndex < MAX_VISIBLE_MENU_ITEMS + 1; slotIndex++) {
        if (!dirtyNavigationRegions[slotIndex].isDirty) {
            continue;
        }

        size_t itemIndex = menuState.firstVisibleIndex + slotIndex - 1;

        RenderRegionScene menuItemScene = {
            .regionId = navigationRegions[slotIndex].id,
            .pixelRegion = navigationRegions[slotIndex].pixelRegion,
            .renderItems = {{0}},
            .count = 0
        };

        if (itemIndex >= menuState.itemCount) {
            menuItemScene.renderItems[menuItemScene.count++] = (PixelRenderItem){ .type = RENDER_ITEM_TYPE_CLEAR };
            displayRenderPlan->regions[displayRenderPlan->count++] = menuItemScene;

            continue;
        }

        struct PixelCoordinates2D textPosition = setMenuItemPosition(navigationRegions[slotIndex], menuState.items[itemIndex]);

        PixelRenderItem menuItemRenderItem = createTextRenderItem(textPosition, menuState.items[itemIndex].label, menuState.items[itemIndex].font);
        menuItemScene.renderItems[menuItemScene.count++] = menuItemRenderItem;

        PixelRenderItem indicatorRenderItem = {0};
        if (setMenuItemIndicator(itemIndex, textPosition, &indicatorRenderItem)) {
            menuItemScene.renderItems[menuItemScene.count++] = indicatorRenderItem;
        }

        displayRenderPlan->regions[displayRenderPlan->count++] = menuItemScene;
    }

    updateMenuSnapshot();
}

void listNavigation_setActiveItem(ListItemId activeItemId) {
    menuState.activeItemId = activeItemId;
    menuState.hasActiveItem = false;

    for (size_t i = 0; i < menuState.itemCount; i++) {
        if (menuState.items[i].id == activeItemId) {
            menuState.hasActiveItem = true;
            break;
        }
    }
}

static void determineDirtyRegions(void) {
    if (isFirstLoad) {
        for (size_t slotIndex = 0; slotIndex < MAX_VISIBLE_MENU_ITEMS + 2; slotIndex++) {
            dirtyNavigationRegions[slotIndex].isDirty = true;
        }
        return;
    }

    for (size_t slotIndex = 0; slotIndex < MAX_VISIBLE_MENU_ITEMS + 2; slotIndex++) {
        dirtyNavigationRegions[slotIndex].isDirty = false;
    }

    if (previousMenuSnapshot.selectedItemIndex != menuState.selectedItemIndex) {
        const bool previousSelectionWasVisible =
            previousMenuSnapshot.selectedItemIndex >= previousMenuSnapshot.firstVisibleIndex &&
            previousMenuSnapshot.selectedItemIndex < previousMenuSnapshot.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS;

        const bool currentSelectionIsVisible =
            menuState.selectedItemIndex >= menuState.firstVisibleIndex &&
            menuState.selectedItemIndex < menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS;

        if (previousSelectionWasVisible) {
            const uint16_t previousSlotIndex =
                previousMenuSnapshot.selectedItemIndex - previousMenuSnapshot.firstVisibleIndex + 1;
            dirtyNavigationRegions[previousSlotIndex].isDirty = true;
        } else {
            ESP_LOGW(TAG, "Previous selection was not visible");
        }

        if (currentSelectionIsVisible) {
            const uint16_t currentSlotIndex =
                menuState.selectedItemIndex - menuState.firstVisibleIndex + 1;
            dirtyNavigationRegions[currentSlotIndex].isDirty = true;
        } else {
            ESP_LOGW(TAG, "Current selection is not visible");
        }
    }

    if (previousMenuSnapshot.activeItemId != menuState.activeItemId) {
        size_t previousActiveIndex = SIZE_MAX;
        size_t currentActiveIndex = SIZE_MAX;

        for (size_t i = 0; i < menuState.itemCount; i++) {
            if (menuState.items[i].id == previousMenuSnapshot.activeItemId) {
                previousActiveIndex = i;
            }

            if (menuState.items[i].id == menuState.activeItemId) {
                currentActiveIndex = i;
            }
        }

        const bool previousActiveWasVisible =
            previousActiveIndex >= previousMenuSnapshot.firstVisibleIndex &&
            previousActiveIndex < previousMenuSnapshot.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS;

        const bool currentActiveIsVisible =
            currentActiveIndex >= menuState.firstVisibleIndex &&
            currentActiveIndex < menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS;

        if (previousActiveWasVisible) {
            const uint16_t previousSlotIndex = previousActiveIndex - previousMenuSnapshot.firstVisibleIndex + 1;
            dirtyNavigationRegions[previousSlotIndex].isDirty = true;
        }
        if (currentActiveIsVisible) {
            const uint16_t currentSlotIndex = currentActiveIndex - menuState.firstVisibleIndex + 1;
            dirtyNavigationRegions[currentSlotIndex].isDirty = true;
        }
    }

    const bool hasMoreBottomItems = menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS < menuState.itemCount;
    const bool hasMoreTopItems = menuState.firstVisibleIndex > 0;

    if (hasMoreBottomItems != previousMenuSnapshot.hasMoreBottomItems) {
        dirtyNavigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].isDirty = true;
    }

    if (hasMoreTopItems != previousMenuSnapshot.hasMoreTopItems) {
        dirtyNavigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].isDirty = true;
    }

    for (size_t slotIndex = 1; slotIndex < MAX_VISIBLE_MENU_ITEMS + 1; slotIndex++) {
        const size_t itemIndex = menuState.firstVisibleIndex + slotIndex - 1;
        const ListItemId previousSlotId = previousMenuSnapshot.itemSnapshots[slotIndex - 1];
        const ListItemId currentSlotId = (itemIndex < menuState.itemCount) ? menuState.items[itemIndex].id : LIST_ITEM_ID_NONE;

        if (currentSlotId != previousSlotId) {
            dirtyNavigationRegions[slotIndex].isDirty = true;
        }
    }
}

static void updateMenuSnapshot(void) {
    previousMenuSnapshot = (ListNavigationSnapshot){
        .selectedItemIndex = menuState.selectedItemIndex,
        .firstVisibleIndex = menuState.firstVisibleIndex,
        .activeItemId = menuState.activeItemId,
        .hasMoreBottomItems = menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS < menuState.itemCount,
        .hasMoreTopItems = menuState.firstVisibleIndex > 0,
        .itemSnapshots = {0}
    };

    for (size_t i = 0; i < MAX_VISIBLE_MENU_ITEMS; i++) {
        size_t itemIndex = menuState.firstVisibleIndex + i;
        
        if (itemIndex >= menuState.itemCount) {
            previousMenuSnapshot.itemSnapshots[i] = LIST_ITEM_ID_NONE;
            continue;
        }

        previousMenuSnapshot.itemSnapshots[i] = menuState.items[itemIndex].id;
    }

    isFirstLoad = false;
}

static void clearNavigationSlots(DisplayRenderPlan *displayRenderPlan) {
    for (size_t slotIndex = 0; slotIndex < MAX_VISIBLE_MENU_ITEMS + 2; slotIndex++) {
        dirtyNavigationRegions[slotIndex].isDirty = true;

        RenderRegionScene clearScene = {
            .regionId = navigationRegions[slotIndex].id,
            .pixelRegion = navigationRegions[slotIndex].pixelRegion,
            .renderItems = {{0}},
            .count = 0
        };

        PixelRenderItem clearItem = {
            .type = RENDER_ITEM_TYPE_CLEAR
        };

        clearScene.renderItems[clearScene.count++] = clearItem;
        displayRenderPlan->regions[displayRenderPlan->count++] = clearScene;
    }
}

static void buildUpIndicatorScene(DisplayRenderPlan *displayRenderPlan) {
    RenderRegionScene upItemScene = {
        .regionId = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].id,
        .pixelRegion = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion,
        .renderItems = {{0}},
        .count = 0
    };

    const char upIndicatorText[32] = "▲";

    struct PixelCoordinates2D upTextPosition = calculateAlignedTextPosition(
        &navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP], 
        upIndicatorText, 
        &Font12, 
        REGION_ALIGNMENT_CENTER
    );

    if (menuState.firstVisibleIndex > 0) {
        PixelRenderItem upTextRenderItem = createTextRenderItem(upTextPosition, upIndicatorText, &Font12);
        upItemScene.renderItems[upItemScene.count++] = upTextRenderItem;
    }

    const uint16_t bottomOfUpItemY = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion.y + navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion.height;    
    
    PixelRenderItem upLineRenderItem = createLineSeparatorRenderItem(
        (struct PixelCoordinates2D){ .x = 0, .y = bottomOfUpItemY - 1 },
        (struct PixelCoordinates2D){ .x = gridConfig.width, .y = bottomOfUpItemY - 1 }
    );

    upItemScene.renderItems[upItemScene.count++] = upLineRenderItem;

    displayRenderPlan->regions[displayRenderPlan->count++] = upItemScene;
} 

static void buildDownIndicatorScene(DisplayRenderPlan *displayRenderPlan) {
    RenderRegionScene downItemScene = {
        .regionId = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].id,
        .pixelRegion = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].pixelRegion,
        .renderItems = {{0}},
        .count = 0
    };

    const char downIndicatorText[32] = "▼";

    struct PixelCoordinates2D downTextPosition = calculateAlignedTextPosition(
        &navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN], 
        downIndicatorText, 
        &Font12, 
        REGION_ALIGNMENT_CENTER
    );

    if (menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS < menuState.itemCount) {
        PixelRenderItem downTextRenderItem = createTextRenderItem(downTextPosition, downIndicatorText, &Font12);
        downItemScene.renderItems[downItemScene.count++] = downTextRenderItem;
    }

    PixelRenderItem downLineRenderItem = createLineSeparatorRenderItem(
        (struct PixelCoordinates2D){ .x = 0, .y = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].pixelRegion.y - 1 },
        (struct PixelCoordinates2D){ .x = gridConfig.width, .y = navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].pixelRegion.y - 1 }
    );
    
    downItemScene.renderItems[downItemScene.count++] = downLineRenderItem;

    displayRenderPlan->regions[displayRenderPlan->count++] = downItemScene;
}

static void updateFirstVisibleIndex(void) {
    if (menuState.selectedItemIndex < menuState.firstVisibleIndex) {
        menuState.firstVisibleIndex = menuState.selectedItemIndex;
        return;
    }
    
    if (menuState.selectedItemIndex >= menuState.firstVisibleIndex + MAX_VISIBLE_MENU_ITEMS) {
        menuState.firstVisibleIndex = menuState.selectedItemIndex - MAX_VISIBLE_MENU_ITEMS + 1;
    }
}

static void setPixelSpace(void) {
    navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].gridRegion = (struct GridRegion){ .x = 0, .y = 0, .width = 1, .height = 1 };
    navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion = (PixelRegion){
        .x = 0,
        .y = 0,
        .width = gridConfig.width,
        .height = Font12.Height + Font12.Height / 2
    };

    for (size_t slotIndex = 1; slotIndex <= MAX_VISIBLE_MENU_ITEMS; slotIndex++) {
        navigationRegions[slotIndex].gridRegion = (struct GridRegion){ .x = 0, .y = slotIndex, .width = 1, .height = 1 };
        navigationRegions[slotIndex].pixelRegion = (PixelRegion){
            .x = 0,
            .y = slotIndex * (gridConfig.height / gridConfig.rows) + navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion.height,
            .width = gridConfig.width,
            .height = gridConfig.height / gridConfig.rows
        };
    }

    navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].gridRegion = (struct GridRegion){ .x = 0, .y = 7, .width = 1, .height = 1 };
    navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_DOWN].pixelRegion = (PixelRegion){
        .x = 0,
        .y = 7 * (gridConfig.height / gridConfig.rows) + navigationRegions[DISPLAY_REGION_SLOT_INDICATOR_UP].pixelRegion.height,
        .width = gridConfig.width,
        .height = Font12.Height + Font12.Height / 2
    };
}

static struct PixelCoordinates2D setMenuItemPosition(DisplayRegionDescriptor region, ListNavigationItem item) {
    struct PixelCoordinates2D position = calculateAlignedTextPosition(
        &region, 
        item.label, 
        item.font, 
        REGION_ALIGNMENT_TOP_LEFT
    );

    return position;
}

static bool setMenuItemIndicator(size_t index, struct PixelCoordinates2D textPosition, PixelRenderItem *indicator) {
    if (index == menuState.selectedItemIndex) {
        *indicator = createTextUnderlineRenderItem(
            textPosition, 
            menuState.items[index].label, 
            menuState.items[index].font, 
            DOT_PIXEL_2X2
        );

        return true;
    }

    if (menuState.hasActiveItem && menuState.items[index].id == menuState.activeItemId) {
        *indicator = createTextUnderlineRenderItem(
            textPosition, 
            menuState.items[index].label, 
            menuState.items[index].font, 
            DOT_PIXEL_1X1
        );

        return true;
    }

    return false;
}

void listNavigation_deinit() {
    menuState = (ListNavigationState){0};
    isFirstLoad = true;
}

