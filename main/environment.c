#include <stdio.h>
#include <stddef.h>

#include "esp_log.h"
#include "shtc3_bsp.h"
#include "pcf85063_bsp.h"

#include "./environment.h"

// Log tag
static const char *TAG = "environment";

static float cacheTemperatureC = 0;
static float cacheRelativeHumidity = 0;

enum env_error initEnvironment(void)
{
    i2c_shtc3_init();

    const int SHTC3Status = SHTC3_GetEnvTemperatureHumidity(&cacheTemperatureC, &cacheRelativeHumidity);
    if (SHTC3Status != 0) {
        ESP_LOGE(TAG, "Failed to init the SHTC3 temperature and humidity sensor!");
        return ENV_FAIL;
    }

    PCF85063_init();

    return ENV_SUCCESS;
}

enum env_error readTemperatureAndHumidity(float *stateTemperatureC, float *stateRelativeHumidity)
{
    ESP_LOGI(TAG,"2.Temperature sensor read...");

    const int SHTC3Status = SHTC3_GetEnvTemperatureHumidity(stateTemperatureC, stateRelativeHumidity);

    if (SHTC3Status != 0) {
        *stateTemperatureC = cacheTemperatureC;
        *stateRelativeHumidity = cacheRelativeHumidity;
        
        ESP_LOGE(TAG, "Failed to read temperature and humidity data from SHTC3 sensor!");

        return ENV_WARNING;
    }

    cacheTemperatureC = *stateTemperatureC;
    cacheRelativeHumidity = *stateRelativeHumidity;

    return ENV_SUCCESS;
}

void readClock(char *clockText, size_t clockTextSize)
{
    const Time_data currentTime = PCF85063_GetTime();
    
    if (currentTime.hours > 23 || currentTime.minutes > 59) {
        snprintf(clockText, clockTextSize, "--:--");
        return;
    }

    snprintf(clockText, clockTextSize, "%02u:%02u", (unsigned)currentTime.hours, (unsigned)currentTime.minutes);
}