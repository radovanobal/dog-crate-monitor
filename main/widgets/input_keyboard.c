#include <stddef.h>

#include "GUI_Paint.h"
#include "epaper_port.h"

#include "input_keyboard.h"
#include "display_types.h"
#include "keyboard_char_map.h"
#include "screen_layout.h"
#include "screen_render.h"
#include "screen_types.h"
#include "app_event.h"
#include "utils/macros.h"


typedef enum {
    INPUT_KEY_TYPE_CHARACTER = 0,
    INPUT_KEY_TYPE_ACTION
} InputKeyType;

typedef struct {
    InputKeyType type;
    union {
        char character;
        KeyboardActionKey actionKey;
    };
} InputKeyboardKey;

static void buildInputScene(DisplayRenderPlan *displayRenderPlan);
static void buildKeyboardScene(DisplayRenderPlan *displayRenderPlan);
static void setPixelSpace(void);
static void confirmUserInput();
static void rebuildKeyboardLookupRegistry();
static void insertCharacterAtCursor(char character);
static void handleKeyboardActionKey(KeyboardActionKey actionKey);
static PixelRenderItem createKeyboardButtonsRenderItem();

static KeyboardLayoutId currentKeyboardLayout = KEYBOARD_LAYOUT_ALPHA;
static size_t activeKeyIndex = 0;
static size_t cursorPosition = 0;
static size_t textLength = 0;

static char currentText[MAX_TEXT_INPUT_LENGTH] = {0};

static KeyboardLayoutId layoutModeOrder[] = {
    KEYBOARD_LAYOUT_ALPHA,
    KEYBOARD_LAYOUT_NUMERIC,
    KEYBOARD_LAYOUT_SPECIAL
};

static InputKeyboardKey keyLookupRegistry[MAX_GRID_CELLS] = {0};

static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 1,
    .rows = 2
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

void inputKeyboard_init(char* storedText) {
    if (storedText == NULL) {
        return;
    }

    strncpy(currentText, storedText, MAX_TEXT_INPUT_LENGTH - 1);
    currentText[MAX_TEXT_INPUT_LENGTH - 1] = '\0';

    textLength = strlen(currentText);
    cursorPosition = textLength; // Start with cursor at the end of the text

    setPixelSpace();
    rebuildKeyboardLookupRegistry();
}

void inputKeyboard_deinit(void) {
    // No dynamic memory to free in this implementation
}

void inputKeyboard_buildRenderPlan(DisplayRenderPlan *renderPlan) {
    if (renderPlan == NULL) {
        return;
    }

    buildInputScene(renderPlan);
    buildKeyboardScene(renderPlan);
}

InputKeyboardResult inputKeyboard_handleInput(InputEventData event) {
    InputKeyboardResult result = {0};

    strncpy(result.text, currentText, sizeof(result.text) - 1);
    result.text[sizeof(result.text) - 1] = '\0';
    
    switch (event.buttonType) {
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_UP:
            result.type = INPUT_KEY_ACTION_TYPE_CURSOR_MOVE;
            activeKeyIndex = (activeKeyIndex + getTotalKeysCountForLayout(currentKeyboardLayout) - 1) % getTotalKeysCountForLayout(currentKeyboardLayout);
            break;
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_DOWN:
            result.type = INPUT_KEY_ACTION_TYPE_CURSOR_MOVE;
            activeKeyIndex = (activeKeyIndex + 1) % getTotalKeysCountForLayout(currentKeyboardLayout);
            break;
        case BUTTON_EVENT_TYPE_ROTARY_ENCODER_PRESS:
            confirmUserInput();

            if (keyLookupRegistry[activeKeyIndex].actionKey == KEYBOARD_SPECIAL_KEY_ENTER) {
                result.type = INPUT_KEY_ACTION_TYPE_CONFIRM;
                break;
            } 
            
            if (keyLookupRegistry[activeKeyIndex].type == INPUT_KEY_TYPE_ACTION) {
                result.type = INPUT_KEY_ACTION_TYPE_ACTION;
                break;
            }

            if (keyLookupRegistry[activeKeyIndex].type == INPUT_KEY_TYPE_CHARACTER) {
                result.type = INPUT_KEY_ACTION_TYPE_CHARACTER;
                break;
            }

            break;
        default:
             ESP_LOGW("InputKeyboard", "Unhandled button event: %d", event.buttonType);
    }

    return result;
}

void inputKeyboard_setText(const char* newText) {
    if (newText == NULL) {
        return;
    }

    strncpy(currentText, newText, MAX_TEXT_INPUT_LENGTH - 1);
    currentText[MAX_TEXT_INPUT_LENGTH - 1] = '\0';

    textLength = strlen(currentText);
    cursorPosition = textLength; // Start with cursor at the end of the text
}

char* inputKeyboard_getText() {
    return currentText;
}

static void rebuildKeyboardLookupRegistry() {
    memset(keyLookupRegistry, 0, sizeof(keyLookupRegistry));

    KeyboardCharacterMap characterMap = getKeyboardCharacterMap(currentKeyboardLayout);
    KeyboardActionKeysMap actionKeysMap = getKeyboardActionKeysMap(currentKeyboardLayout);

    for (size_t i = 0; i < characterMap.count; i++) {
        keyLookupRegistry[i].type = INPUT_KEY_TYPE_CHARACTER;
        keyLookupRegistry[i].character = characterMap.characters[i];
    }

    for (size_t i = 0; i < actionKeysMap.count; i++) {
        const size_t cellIndex = characterMap.count + i;

        keyLookupRegistry[cellIndex].type = INPUT_KEY_TYPE_ACTION;
        keyLookupRegistry[cellIndex].actionKey = actionKeysMap.actionKeys[i];
    }
}

static void setPixelSpace(void) {
    inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].gridRegion = (struct GridRegion){ .x = 0, .y = 0, .width = 1, .height = 1 };
    inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion = (PixelRegion){
        .x = 0,
        .y = 0,
        .width = gridConfig.width,
        .height = gridConfig.height / 6
    };

    inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].gridRegion = (struct GridRegion){ .x = 0, .y = 1, .width = 1, .height = 1 };
    inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion = (PixelRegion){
        .x = 0,
        .y = gridConfig.height / 6,
        .width = gridConfig.width,
        .height = gridConfig.height * 5 / 6
    };
}

static void confirmUserInput() {
    switch (keyLookupRegistry[activeKeyIndex].type) {
        case INPUT_KEY_TYPE_CHARACTER:
            insertCharacterAtCursor(keyLookupRegistry[activeKeyIndex].character);
            break;
        case INPUT_KEY_TYPE_ACTION:
            handleKeyboardActionKey(keyLookupRegistry[activeKeyIndex].actionKey);
            break;
        default:
            ESP_LOGW("InputKeyboard", "Unhandled key type: %d", keyLookupRegistry[activeKeyIndex].type);
    }
}

static void insertCharacterAtCursor(char character) {
    if (textLength >= MAX_TEXT_INPUT_LENGTH - 1) {
        return; // No space to insert new character
     }

    // Shift characters to the right of the cursor to make space for the new character
    for (size_t i = textLength; i > cursorPosition; i--) {
        currentText[i] = currentText[i - 1];
    }

    currentText[cursorPosition] = character;
    textLength++;
    cursorPosition++;
}

static void handleKeyboardActionKey(KeyboardActionKey actionKey) {
    switch (actionKey) {
        case KEYBOARD_SPECIAL_KEY_BACKSPACE:
            if (cursorPosition > 0) {
                // Shift characters to the left to delete the character before the cursor
                for (size_t i = cursorPosition - 1; i < textLength - 1; i++) {
                    currentText[i] = currentText[i + 1];
                }
                currentText[textLength - 1] = '\0';
                textLength--;
                cursorPosition--;
            }
            break;
        case KEYBOARD_SPECIAL_KEY_CLEAR:
            currentText[0] = '\0';
            textLength = 0;
            cursorPosition = 0;
            break;
        case KEYBOARD_SPECIAL_KEY_ENTER:
            break;
        case KEYBOARD_SPECIAL_KEY_SPACE:
            insertCharacterAtCursor(' ');
            break;
        case KEYBOARD_SPECIAL_KEY_SHIFT:
            // Handle shift key action, e.g., toggle between uppercase and lowercase
            break;
        case KEYBOARD_SPECIAL_KEY_MODE:
            currentKeyboardLayout = layoutModeOrder[(size_t)(currentKeyboardLayout + 1) % ARRAY_SIZE(layoutModeOrder)];
            rebuildKeyboardLookupRegistry();
            break;
        case KEYBOARD_SPECIAL_KEY_CURSOR_LEFT:
            if (cursorPosition > 0) {
                cursorPosition--;
            }
            break;
        case KEYBOARD_SPECIAL_KEY_CURSOR_RIGHT:
            if (cursorPosition < textLength) {
                cursorPosition++;
            }
            break;
        default:
             ESP_LOGW("InputKeyboard", "Unhandled action key: %d", actionKey);
    }
}

static void buildInputScene(DisplayRenderPlan *displayRenderPlan) {
     RenderRegionScene inputTextScene = {
        .regionId = inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].id,
        .pixelRegion = inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion,
        .renderItems = {{0}},
        .count = 0
    };

    struct PixelCoordinates2D textPosition = calculateAlignedTextPosition(
        &inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT], 
        currentText, 
        &Font16, 
        REGION_ALIGNMENT_CENTER_LEFT
    );

    PixelRenderItem textRenderItem = createTextRenderItem(textPosition, currentText, &Font16);
    inputTextScene.renderItems[inputTextScene.count++] = textRenderItem;

    PixelRenderItem separatorLine = createLineSeparatorRenderItem(
        (struct PixelCoordinates2D){ .x = 0, .y = inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion.y + inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion.height - 1 },
        (struct PixelCoordinates2D){ .x = gridConfig.width, .y = inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion.y + inputKeyboardRegions[DISPLAY_REGION_SLOT_INPUT_TEXT].pixelRegion.height - 1 }
    );

    inputTextScene.renderItems[inputTextScene.count++] = separatorLine;

    displayRenderPlan->regions[displayRenderPlan->count++] = inputTextScene;
}

static void buildKeyboardScene(DisplayRenderPlan *displayRenderPlan) {
    RenderRegionScene keyboardScene = {
        .regionId = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].id,
        .pixelRegion = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion,
        .renderItems = {{0}},
        .count = 0
    };

    PixelRenderItem keyboardBackground = createBoxItem(
        (struct PixelCoordinates2D){ .x = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.x, .y = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.y },
        (struct PixelSize2D){ .width = gridConfig.width, .height = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.height },
        DOT_PIXEL_1X1,
        DRAW_FILL_EMPTY
    );

    keyboardScene.renderItems[keyboardScene.count++] = keyboardBackground;

    PixelRenderItem keyboardButtons = createKeyboardButtonsRenderItem();
    keyboardScene.renderItems[keyboardScene.count++] = keyboardButtons;

    displayRenderPlan->regions[displayRenderPlan->count++] = keyboardScene;
}

static PixelRenderItem createKeyboardButtonsRenderItem() {
    PixelRenderItem renderItem = {
        .type = RENDER_ITEM_TYPE_GRID,
        .data = {
            .grid = {
                .position = (struct PixelCoordinates2D){ .x = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.x, .y = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.y },
                .size = (struct PixelSize2D){ .width = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.width, .height = inputKeyboardRegions[DISPLAY_REGION_SLOT_KEYBOARD].pixelRegion.height },
                .colorAccent = DISPLAY_COLOR_BLACK,
                .colorDefault = DISPLAY_COLOR_WHITE,
                .rows = 5,
                .columns = 14,
                .cells = {0} // Cell data will be populated dynamically based on the keyboard layout
            }
        }
    };

    if (ARRAY_SIZE(keyLookupRegistry) > ARRAY_SIZE(renderItem.data.grid.cells)) {
        ESP_LOGW("InputKeyboard", "Key lookup registry size exceeds grid cell capacity. Some keys will not be rendered.");
    }

    for (size_t i = 0; i < ARRAY_SIZE(keyLookupRegistry); i++) {
        if (keyLookupRegistry[i].type == INPUT_KEY_TYPE_CHARACTER) {
            renderItem.data.grid.cells[i].isActive = i == activeKeyIndex;
            renderItem.data.grid.cells[i].text[0] = keyLookupRegistry[i].character;
            renderItem.data.grid.cells[i].text[1] = '\0';
            renderItem.data.grid.cells[i].columnSpan = 1;
        } else if (keyLookupRegistry[i].type == INPUT_KEY_TYPE_ACTION) {
            renderItem.data.grid.cells[i].isActive = i == activeKeyIndex;
            strncpy(renderItem.data.grid.cells[i].text, getKeyboardActionKeyDisplayText(keyLookupRegistry[i].actionKey), sizeof(renderItem.data.grid.cells[i].text) - 1);
            renderItem.data.grid.cells[i].text[sizeof(renderItem.data.grid.cells[i].text) - 1] = '\0';
            renderItem.data.grid.cells[i].columnSpan = 2;
        }
    }

    return renderItem;
}