#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bsp.h"


#include "./display.h"
#include "./environment.h"


// Log tag
// static const char *TAG = "main";

static float stateTemperatureC;
static float stateRelativeHumidity;
static char clockText[16] = "--:--";

static void initCommunications(void);

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
        readClock(clockText, sizeof(clockText));

        if (envStatus != ENV_FAIL) {
            renderToDisplay(stateTemperatureC, stateRelativeHumidity, clockText);
        }

        if (envStatus == ENV_SUCCESS) {
            vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute before the next reading
        } else if(envStatus == ENV_WARNING) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 ms
        } else {
            // Environment read failed, wait for a while before retrying
            vTaskDelay(pdMS_TO_TICKS(300)); // Wait 300 ms before trying sensor read
        }
    }
}

static void initCommunications(void)
{
    i2c_master_init();
}