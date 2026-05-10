#ifndef DOG_CRATE_MONITOR_INPUT_KEYBOARD_H
#define DOG_CRATE_MONITOR_INPUT_KEYBOARD_H

#include "display/display_types.h"
#include "app/app_event.h"

#define MAX_TEXT_INPUT_LENGTH 65

typedef enum {
    INPUT_KEY_ACTION_TYPE_CHARACTER,
    INPUT_KEY_ACTION_TYPE_CURSOR_MOVE,
    INPUT_KEY_ACTION_TYPE_CONFIRM,
    INPUT_KEY_ACTION_TYPE_ACTION
} InputKeyboardActionType;

typedef struct {
    InputKeyboardActionType type;
    char text[MAX_TEXT_INPUT_LENGTH];
} InputKeyboardResult;

void inputKeyboard_init(char* storedText);
void inputKeyboard_deinit(void);
void inputKeyboard_buildRenderPlan(DisplayRenderPlan *renderPlan);
InputKeyboardResult inputKeyboard_handleInput(InputEventData event);
void inputKeyboard_setText(const char* newText);
char* inputKeyboard_getText(void);

#endif // DOG_CRATE_MONITOR_INPUT_KEYBOARD_H