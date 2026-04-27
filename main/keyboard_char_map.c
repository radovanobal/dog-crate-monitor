#include "keyboard_char_map.h"
#include "./utils/macros.h"
#include <stddef.h>

static const char numbers[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

static const char alphaRows[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u',
    'v', 'w', 'x', 'y', 'z'
};

static const char specialChars[] = {
    ' ', '!', '"', '#', '$', '%', '&', 
    '\'', '(', ')', '*', '+', ',', '-', 
    '.', '/', ':', ';', '<', '=', '>',
    '?', '@', '[', '\\', ']', '^', '_',
    '`', '{', '|', '}', '~'
};

static const KeyboardActionKey actionKeys[] = {
    KEYBOARD_SPECIAL_KEY_SPACE,
    KEYBOARD_SPECIAL_KEY_SHIFT,
    KEYBOARD_SPECIAL_KEY_MODE,
    KEYBOARD_SPECIAL_KEY_BACKSPACE,
    KEYBOARD_SPECIAL_KEY_ENTER,
    KEYBOARD_SPECIAL_KEY_CLEAR,
    KEYBOARD_SPECIAL_KEY_CURSOR_LEFT,
    KEYBOARD_SPECIAL_KEY_CURSOR_RIGHT,
};

KeyboardCharacterMap getKeyboardCharacterMap(KeyboardLayoutId layoutId) {
    KeyboardCharacterMap map = {0};

    switch (layoutId) {
        case KEYBOARD_LAYOUT_ALPHA:
            map.characters = alphaRows;
            map.count = ARRAY_SIZE(alphaRows);
            break;
        case KEYBOARD_LAYOUT_NUMERIC:
            map.characters = numbers;
            map.count = ARRAY_SIZE(numbers);
            break;
        case KEYBOARD_LAYOUT_SPECIAL:
            map.characters = specialChars;
            map.count = ARRAY_SIZE(specialChars);
            break;    
        default:
            break;
    }

    return map;
}

// TODO: layout ID may need to filter some action keys in the future if certain keys are not relevant for specific layouts; if not remove it
KeyboardActionKeysMap getKeyboardActionKeysMap(KeyboardLayoutId layoutId) { 
    KeyboardActionKeysMap map = {
        .actionKeys = actionKeys,
        .count = ARRAY_SIZE(actionKeys)
    };

    return map;
}

size_t getTotalKeysCountForLayout(KeyboardLayoutId layoutId) {
    KeyboardCharacterMap characterMap = getKeyboardCharacterMap(layoutId);
    KeyboardActionKeysMap actionKeysMap = getKeyboardActionKeysMap(layoutId);

    return characterMap.count + actionKeysMap.count;
}

const char *getKeyboardActionKeyDisplayText(KeyboardActionKey actionKey) {
    switch (actionKey) {
        case KEYBOARD_SPECIAL_KEY_SPACE:
            return "Space";
        case KEYBOARD_SPECIAL_KEY_SHIFT:
            return "Shift";
        case KEYBOARD_SPECIAL_KEY_MODE:
            return "Mode";
        case KEYBOARD_SPECIAL_KEY_BACKSPACE:
            return "Delete";
        case KEYBOARD_SPECIAL_KEY_ENTER:
            return "Enter";
        case KEYBOARD_SPECIAL_KEY_CURSOR_LEFT:
            return "Left";
        case KEYBOARD_SPECIAL_KEY_CURSOR_RIGHT:
            return "Right";
        case KEYBOARD_SPECIAL_KEY_CLEAR:
            return "Clear";
        default:
            return "";
    }
}