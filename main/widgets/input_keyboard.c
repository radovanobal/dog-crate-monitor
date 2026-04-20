#include "epaper_port.h"

#include "input_keyboard.h"
#include "display_types.h"
#include "screen_types.h"

static char* currentText = NULL;
static size_t maxTextLength = 32;

static void buildInputScene(DisplayRenderPlan *displayRenderPlan);
static void buildKeyboardScene(DisplayRenderPlan *displayRenderPlan);

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 16
};

enum {
    DISPLAY_REGION_SLOT_INPUT_TEXT = 0,
    DISPLAY_REGION_SLOT_KEYBOARD
};

static DisplayRegionDescriptor inputKeyboardRegions[] = {
    [DISPLAY_REGION_SLOT_INPUT_TEXT] = { .id = DISPLAY_REGION_SLOT_INPUT_TEXT },
    [DISPLAY_REGION_SLOT_KEYBOARD] = { .id = DISPLAY_REGION_SLOT_KEYBOARD }
};

static DirtyRegionEntry dirtyInputKeyboardRegions[] = {
    [DISPLAY_REGION_SLOT_INPUT_TEXT] = { .regionId = DISPLAY_REGION_SLOT_INPUT_TEXT, .isDirty = false },
    [DISPLAY_REGION_SLOT_KEYBOARD] = { .regionId = DISPLAY_REGION_SLOT_KEYBOARD, .isDirty = false }
};

void inputKeyboard_init(char* storedText, size_t maxLength) {
    if (storedText == NULL || maxLength == 0) {
        return;
    }

    currentText = storedText;
    maxTextLength = maxLength;
}

void inputKeyboard_deinit(void) {
    // No dynamic memory to free in this implementation
}

void inputKeyboard_buildRenderPlan(DisplayRenderPlan *renderPlan) {
    if (renderPlan == NULL) {
        return;
    }
}

void inputKeyboard_setText(const char* newText) {
    if (newText == NULL) {
        return;
    }

    strncpy(currentText, newText, maxTextLength - 1);
}

char* inputKeyboard_getText() {
    return currentText;
}

static void buildInputScene(DisplayRenderPlan *displayRenderPlan) {

}