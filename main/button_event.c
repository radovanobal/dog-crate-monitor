
#include <stdbool.h>

#include "button_bsp.h"
#include "esp_log.h"

#include "./button_event.h"

// Log tag
static const char *TAG = "button_event";
static const EventBits_t wakeButtonBits = set_bit_button(0) 
    | set_bit_button(7) 
    | set_bit_button(14) 
    | set_bit_button(21);

void buttonEvent_init(void) {
    button_Init();    
}

bool buttonEvent_wait(ButtonEvent *event, TickType_t ticksToWait) {
    if(event == NULL) {
        ESP_LOGE(TAG, "buttonEvent_wait called with NULL event pointer");
        return false;
    }

    EventBits_t keyEventBits = xEventGroupWaitBits(
        key_groups, 
        wakeButtonBits, // Wake button bits for all buttons
        pdTRUE, 
        pdFALSE,
        ticksToWait
    );

    EventBits_t relevantBits = keyEventBits & wakeButtonBits;

    if (relevantBits == 0) {
        return false;
    }

    event->stepCount = 1; // Step count is not used in current implementation, set to 1

    if(relevantBits & set_bit_button(0)) {
        event->pressType = BUTTON_PRESS_TYPE_SINGLE_CLICK;
        event->buttonType = BUTTON_EVENT_TYPE_ROTARY_ENCODER_UP;
        return true;
    }

    if (relevantBits & set_bit_button(7)) {
        event->pressType = BUTTON_PRESS_TYPE_SINGLE_CLICK;
        event->buttonType = BUTTON_EVENT_TYPE_ROTARY_ENCODER_PRESS;
        return true;
    }

    if (relevantBits & set_bit_button(14)) {
        event->pressType = BUTTON_PRESS_TYPE_SINGLE_CLICK;
        event->buttonType = BUTTON_EVENT_TYPE_ROTARY_ENCODER_DOWN;
        return true;
    }

    if (relevantBits & set_bit_button(21)) {
        event->pressType = BUTTON_PRESS_TYPE_SINGLE_CLICK;
        event->buttonType = BUTTON_EVENT_TYPE_BUTTON_SELECT;
        return true;
    }

    ESP_LOGW(TAG, "Received button event bits that do not match any known button: 0x%08X", relevantBits);
    return false;
}