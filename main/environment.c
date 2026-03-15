#include <stdio.h>
#include <stddef.h>

#include "esp_log.h"
#include "shtc3_bsp.h"
#include "pcf85063_bsp.h"

#include "./environment_types.h"
#include "./environment.h"

// Log tag
static const char *TAG = "environment";

static float cacheTemperatureC = 0;
static float cacheRelativeHumidity = 0;
static int readTryCount = 0;

int getReadTryCount() {
    return readTryCount;
}

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
    readTryCount++;

    const int SHTC3Status = SHTC3_GetEnvTemperatureHumidity(stateTemperatureC, stateRelativeHumidity);

    if(readTryCount > 5) {
        return ENV_FAIL;
    }

    if (SHTC3Status != 0) {
        *stateTemperatureC = cacheTemperatureC;
        *stateRelativeHumidity = cacheRelativeHumidity;
        
        ESP_LOGE(TAG, "Failed to read temperature and humidity data from SHTC3 sensor!");

        return ENV_WARNING;
    }

    readTryCount = 0;
    cacheTemperatureC = *stateTemperatureC;
    cacheRelativeHumidity = *stateRelativeHumidity;

    return ENV_SUCCESS;
}

enum env_error getCurrentTime(TimeDate *currentTime)
{
    Time_data currentTimeRead = PCF85063_GetTime();

    if (currentTimeRead.hours > 23 || currentTimeRead.minutes > 59) {
        ESP_LOGE(TAG, "Failed to read current time from PCF85063 RTC!");
        return ENV_FAIL;
    }

    *currentTime = (TimeDate){
        .years = currentTimeRead.years,
        .months = currentTimeRead.months,
        .days = currentTimeRead.days,
        .hours = currentTimeRead.hours,
        .minutes = currentTimeRead.minutes,
        .seconds = currentTimeRead.seconds,
        .week = currentTimeRead.week
    };

    return ENV_SUCCESS;
}