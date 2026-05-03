#include <stdbool.h>

#include "axp_prot.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "power_management.h"
#include "button_event.h"
#include "task_drain_events.h"

static const char *TAG = "power_management";

static bool isPreparingForTaskSuspension = false;
static bool isPowerOffInitiated = false;
static bool isSleepModeInitiated = false;

static void drainCompleteCallback (TaskDrainState state);

static const uint64_t wakeMask =
    (1ULL << GPIO_NUM_0) |
    (1ULL << GPIO_NUM_4) |
    (1ULL << GPIO_NUM_5) |
    (1ULL << GPIO_NUM_6);


void powerManagement_init(void) {
    taskDrainMonitor_registerDrainCompleteCallback(drainCompleteCallback);
}

void powerManagement_handlePowerButtonInput(ButtonEvent buttonEvent) {
    if (buttonEvent.buttonType != BUTTON_EVENT_TYPE_POWER) {
        ESP_LOGW(TAG, "Received non-power button event in powerManagement_handlePowerButtonInput: %d", buttonEvent.buttonType);
        
        return;
    }

    isPreparingForTaskSuspension = true;

    if (buttonEvent.pressType == BUTTON_PRESS_TYPE_LONG_PRESS) {
        isPowerOffInitiated = true;
        ESP_LOGI(TAG, "Power button long press detected - initiating power off sequence");
    }

    if (buttonEvent.pressType == BUTTON_PRESS_TYPE_SHORT_PRESS) {
        isSleepModeInitiated = true;
        ESP_LOGI(TAG, "Power button double click detected - initiating sleep mode sequence");
    }
}

void powerManagement_abortTaskSuspend(void) {
    if (isPreparingForTaskSuspension) {
        isPreparingForTaskSuspension = false;
        isPowerOffInitiated = false;
        isSleepModeInitiated = false;

        ESP_LOGI(TAG, "Task suspension aborted");
    }
}

bool powerManagement_isPreparingForTaskSuspension(void) {
    return isPreparingForTaskSuspension;
}

static void drainCompleteCallback (TaskDrainState state) {
    if (state != TASK_DRAIN_STATE_COMPLETE) {
        return;
    }

    if (state == TASK_DRAIN_STATE_COMPLETE && isPowerOffInitiated) {
        ESP_LOGI(TAG, "All drains are empty, proceeding with power off");
        axp_pwr_off();

        return;
    }

    if (state == TASK_DRAIN_STATE_COMPLETE && isSleepModeInitiated) {
        ESP_LOGI(TAG, "All drains are empty, proceeding with sleep mode");

        ESP_ERROR_CHECK(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL));
        ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
            wakeMask,
            ESP_EXT1_WAKEUP_ANY_LOW
        ));

        esp_deep_sleep_start();

        return;
    }
};