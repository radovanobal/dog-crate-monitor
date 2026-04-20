#ifndef DOG_CRATE_MONITOR_INPUT_KEYBOARD_H
#define DOG_CRATE_MONITOR_INPUT_KEYBOARD_H

#include "../display_types.h"
#include "../app_event.h"

void inputKeyboard_init(char* storedText, size_t maxTextLength);
void inputKeyboard_deinit(void);
void inputKeyboard_buildRenderPlan(DisplayRenderPlan *renderPlan);
void inputKeyboard_handleInput(InputEventData event);
void inputKeyboard_setText(const char* newText);
char* inputKeyboard_getText(void);

#endif // DOG_CRATE_MONITOR_INPUT_KEYBOARD_H