#ifndef DOG_CRATE_MONITOR_BUTTON_EVENT_H
#define DOG_CRATE_MONITOR_BUTTON_EVENT_H

#include <stdbool.h>

#include "freertos/FreeRTOS.h"

typedef enum {
    BUTTON_EVENT_TYPE_NONE = 0,
    BUTTON_EVENT_TYPE_ROTARY_ENCODER_UP = 1,
    BUTTON_EVENT_TYPE_ROTARY_ENCODER_DOWN = 2,
    BUTTON_EVENT_TYPE_ROTARY_ENCODER_PRESS = 3,
    BUTTON_EVENT_TYPE_BUTTON_SELECT = 4, // Hardware button label: boot
} ButtonType;

typedef enum {
    BUTTON_PRESS_TYPE_SINGLE_CLICK = 0,
    BUTTON_PRESS_TYPE_DOUBLE_CLICK = 1,
    BUTTON_PRESS_TYPE_LONG_PRESS = 2,
    BUTTON_PRESS_TYPE_PRESS_UP = 3,
    BUTTON_PRESS_TYPE_PRESS_DOWN = 4,
} ButtonPressType;

typedef struct {
    ButtonType buttonType;
    ButtonPressType pressType;
    uint8_t stepCount;
} ButtonEvent;

void buttonEvent_init(void);
bool buttonEvent_wait(ButtonEvent *event, TickType_t ticksToWait);


#endif // DOG_CRATE_MONITOR_BUTTON_EVENT_H