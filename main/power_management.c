#include "axp_prot.h"
#include "esp_log.h"

#include "power_management.h"
#include "button_event.h"
#include "task_drain_events.h"

static const char *TAG = "PowerManagement";

static bool isPreparingForSleep = false;

static void drainCompleteCallback (TaskDrainState state);


void powerManagement_init(void) {

    taskDrainMonitor_registerDrainCompleteCallback(drainCompleteCallback);
}

void powerManagement_handlePowerButtonInput(ButtonEvent buttonEvent) {
    if (buttonEvent.buttonType != BUTTON_EVENT_TYPE_POWER) {
        ESP_LOGW(TAG, "Received non-power button event in powerManagement_handlePowerButtonInput: %d", buttonEvent.buttonType);
        return;
    }

    if (buttonEvent.pressType == BUTTON_PRESS_TYPE_SHORT_PRESS) {
        isPreparingForSleep = true;
        ESP_LOGI(TAG, "Power button short press detected - preparing for power off");
    }
}

void powerManagement_abortPowerOff(void) {
    if (isPreparingForSleep) {
        isPreparingForSleep = false;
        ESP_LOGI(TAG, "Power off aborted");
    }
}

bool powerManagement_isPreparingForPowerOff(void) {
    return isPreparingForSleep;
}

static void drainCompleteCallback (TaskDrainState state) {
    if (state == TASK_DRAIN_STATE_COMPLETE && isPreparingForSleep) {
        ESP_LOGI(TAG, "All drains are empty, proceeding with power off");
        axp_pwr_off();
    }
};