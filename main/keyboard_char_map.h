#ifndef DOG_CRATE_MONITOR_KEYBOARD_CHAR_MAP_H
#define DOG_CRATE_MONITOR_KEYBOARD_CHAR_MAP_H

#include <stddef.h>

typedef enum {
    KEYBOARD_SPECIAL_KEY_SPACE,
    KEYBOARD_SPECIAL_KEY_SHIFT,
    KEYBOARD_SPECIAL_KEY_MODE,
    KEYBOARD_SPECIAL_KEY_BACKSPACE,
    KEYBOARD_SPECIAL_KEY_CLEAR,
    KEYBOARD_SPECIAL_KEY_ENTER,
    KEYBOARD_SPECIAL_KEY_CURSOR_LEFT,
    KEYBOARD_SPECIAL_KEY_CURSOR_RIGHT,
} KeyboardActionKey;

typedef enum {
    KEYBOARD_LAYOUT_ALPHA,
    KEYBOARD_LAYOUT_NUMERIC,
    KEYBOARD_LAYOUT_SPECIAL
} KeyboardLayoutId;

typedef struct {
    const char *characters;
    size_t count;
} KeyboardCharacterMap;

typedef struct {
    const KeyboardActionKey *actionKeys;
    size_t count;
} KeyboardActionKeysMap;

KeyboardCharacterMap getKeyboardCharacterMap(KeyboardLayoutId layoutId);
KeyboardActionKeysMap getKeyboardActionKeysMap(KeyboardLayoutId layoutId);
const char *getKeyboardActionKeyDisplayText(KeyboardActionKey actionKey);
size_t getTotalKeysCountForLayout(KeyboardLayoutId layoutId);

#endif // DOG_CRATE_MONITOR_KEYBOARD_CHAR_MAP_H