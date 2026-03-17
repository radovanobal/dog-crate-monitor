#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bsp.h"

#include "./display_types.h"
#include "./display.h"
#include "./environment_types.h"
#include "./environment.h"


// Log tag
// static const char *TAG = "main";

static float stateTemperatureC;
static float stateRelativeHumidity;
static TimeDate currentTime;
static int partialRenderCount = 0;
static int maxPartialRenderCount = 100;
static bool isFullRenderRequired = true;

static void initCommunications(void);
static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate currentTime);
static int max(int a, int b);

void app_main(void)
{
    initCommunications();

    if(initEnvironment() != ENV_SUCCESS) {
        return;
    }
    
    if (initDisplay() != DISPLAY_SUCCESS) {
        return;
    }

    
    while(true) {
        enum env_error envStatus = readTemperatureAndHumidity(&stateTemperatureC, &stateRelativeHumidity);
        enum env_error timeStatus = getCurrentTime(&currentTime);
        
        enum env_error status = max(envStatus, timeStatus);
        
        if (status != ENV_FAIL) {
            DisplayState displayState = readyDisplayState(stateTemperatureC, stateRelativeHumidity, currentTime);

            if (isFullRenderRequired) {
                renderToDisplay(&displayState);
                isFullRenderRequired = false;
            } else {
                partialRenderToDisplay(&displayState);
            }

            partialRenderCount++;

            if(partialRenderCount >= maxPartialRenderCount) {
                partialRenderCount = 0;
                isFullRenderRequired = true;
            }
        }

        if (status == ENV_SUCCESS) {
            vTaskDelay(pdMS_TO_TICKS(15000)); // Delay for 15 seconds before the next reading
        } else if(status == ENV_WARNING) {
            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 2s
        } else {
            if (getReadTryCount() > 4) {
                // fail slow
                vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5s before trying sensor read
            } else {
                // fail fast
                // Environment read failed, wait for a while before retrying
                vTaskDelay(pdMS_TO_TICKS(300)); // Wait 300 ms before trying sensor read
            }
        }
    }
}

static void initCommunications(void)
{
    i2c_master_init();
}

static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate currentTime) {
    DisplayState displayState = {0};
    snprintf(displayState.temperatureText, sizeof(displayState.temperatureText), "%.1fC", stateTemperatureC);
    snprintf(displayState.humidityText, sizeof(displayState.humidityText), "%.1f%%", stateRelativeHumidity);

    if (currentTime.hours > 23 || currentTime.minutes > 59) {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "--:--");
    } else {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "%02u:%02u", (unsigned)currentTime.hours, (unsigned)currentTime.minutes);
    }

    displayState.showEnvironmentWarning = false;
    displayState.showBluetooth = false;
    displayState.showWifi = false;
    displayState.showBattery = false;

    return displayState;
}

static int max(int a, int b) {
    return (a > b) ? a : b;
}