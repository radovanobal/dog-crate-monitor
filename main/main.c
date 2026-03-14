#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bsp.h"
#include "shtc3_bsp.h"
#include "pcf85063_bsp.h"

#include "./display.h"

static esp_err_t initI2C(void);
static esp_err_t readTemperatureAndHumidity(void);
static void readClock(char *clockText, size_t clockTextSize);

// Log tag
static const char *TAG = "main";

static float stateTemparatureC;
static float stateRelativeHumidity;
static char clockText[16] = "--:--";

void app_main(void)
{
    
    if(initI2C() != ESP_OK) {
        return;
    }
    
    if (initDisplay() != ESP_OK) {
        return;
    }

    while(true) {
        if (readTemperatureAndHumidity() == ESP_OK) {
            readClock(clockText, sizeof(clockText));
            renderToDisplay(stateTemparatureC, stateRelativeHumidity, clockText);
            vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute before the next reading
        } else {
            vTaskDelay(pdMS_TO_TICKS(300)); // Wait 300 ms before trying sensor read
        }
    }
}

static esp_err_t initI2C(void)
{
    i2c_master_init();
    i2c_shtc3_init();

    const int SHTC3Status = SHTC3_GetEnvTemperatureHumidity(&stateTemparatureC, &stateRelativeHumidity);
    if (SHTC3Status != 0) {
        ESP_LOGE(TAG, "Failed to init the SHTC3 temperature and humidity sensor!");
        return ESP_FAIL;
    }

    PCF85063_init();

    return ESP_OK;
}

static esp_err_t readTemperatureAndHumidity(void)
{
    ESP_LOGI(TAG,"2.Temperature sensor read...");

    float readTemperatureC;
    float readRelativeHumidity;

    const int SHTC3Status = SHTC3_GetEnvTemperatureHumidity(&readTemperatureC, &readRelativeHumidity);

    if (SHTC3Status == 0) {
        stateTemparatureC = readTemperatureC;
        stateRelativeHumidity = readRelativeHumidity;

        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to read temperature and humidity data from SHTC3 sensor!");
    return ESP_FAIL;
}

static void readClock(char *clockText, size_t clockTextSize)
{
    const Time_data currentTime = PCF85063_GetTime();
    
    if (currentTime.hours > 23 || currentTime.minutes > 59) {
        snprintf(clockText, clockTextSize, "--:--");
        return;
    }

    snprintf(clockText, clockTextSize, "%02d:%02d", currentTime.hours, currentTime.minutes);
}