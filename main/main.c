#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "i2c_bsp.h"

#include "./utils.h"
#include "./display_types.h"
#include "./display.h"
#include "./environment_types.h"
#include "./environment.h"


// Log tag
static const char *TAG = "main";

static int maxPartialRenderCount = 100;

static void initCommunications(void);
static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate currentTime);


typedef enum {
    DISPLAY_PAINT_TYPE_FULL,
    DISPLAY_PAINT_TYPE_PARTIAL
} DisplayPaintType;

typedef enum {
    APP_EVENT_ENVIRONMENT_UPDATED,
} AppEventType;

typedef struct {
    DisplayPaintType paintType;
//    DisplayRegionId regionId;
    DisplayState displayState;
} DisplayRequest;

typedef struct {
    AppEventType eventType;
    struct {
        float temperatureC;
        float relativeHumidity;
    } sensorUpdateData;
    TimeDate timeUpdateData;
} AppEvent;

static QueueHandle_t displayQueue = NULL;
static QueueHandle_t appEventQueue = NULL;

static void uiTask(void *pvParameters);
static void renderTask(void *pvParameters);
static void serviceTask(void *pvParameters);

void app_main(void)
{
    initCommunications();

    if(initEnvironment() != ENV_SUCCESS) {
        return;
    }
    
    if (initDisplay() != DISPLAY_SUCCESS) {
        return;
    }

    displayQueue = xQueueCreate(10, sizeof(DisplayRequest));
    appEventQueue = xQueueCreate(10, sizeof(AppEvent));

    if (displayQueue == NULL || appEventQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }

    if(xTaskCreatePinnedToCore(uiTask, "uiTask", 4096, NULL, 5, NULL, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return;
    }   

    if(xTaskCreatePinnedToCore(renderTask, "renderTask", 4096, NULL, 4, NULL, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Render task");
        return;
    }

    if(xTaskCreatePinnedToCore(serviceTask, "serviceTask", 4096, NULL, 3, NULL, 1) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Service task");
        return;
    }
}

static void initCommunications(void)
{
    i2c_master_init();
}

static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate stateCurrentTime) {
    DisplayState displayState = {0};
    snprintf(displayState.temperatureText, sizeof(displayState.temperatureText), "%.1fC", stateTemperatureC);
    snprintf(displayState.humidityText, sizeof(displayState.humidityText), "%.1f%%", stateRelativeHumidity);

    if (stateCurrentTime.hours > 23 || stateCurrentTime.minutes > 59) {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "--:--");
    } else {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "%02u:%02u", (unsigned)stateCurrentTime.hours, (unsigned)stateCurrentTime.minutes);
    }

    displayState.showEnvironmentWarning = false;
    displayState.showBluetooth = false;
    displayState.showWifi = false;
    displayState.showBattery = false;

    return displayState;
}

static void uiTask(void *pvParameters) {
    AppEvent event;
    DisplayState currentDisplayState = {0};
    int partialRenderCount = maxPartialRenderCount;
 
    for(;;) {
        if(xQueueReceive(appEventQueue, &event, portMAX_DELAY) == pdPASS) {
            switch (event.eventType) {
                case APP_EVENT_ENVIRONMENT_UPDATED:
                    currentDisplayState = readyDisplayState(
                        event.sensorUpdateData.temperatureC, 
                        event.sensorUpdateData.relativeHumidity, 
                        event.timeUpdateData
                    );
                    break;
            }

            bool isFullRenderRequired = partialRenderCount >= maxPartialRenderCount;
            DisplayPaintType paintType = DISPLAY_PAINT_TYPE_PARTIAL;
            if (isFullRenderRequired) {
                partialRenderCount = 0;
                paintType = DISPLAY_PAINT_TYPE_FULL;
            }
            
            DisplayRequest displayRequest = {
                .paintType = paintType,
                .displayState = currentDisplayState
            };

            if(xQueueSend(displayQueue, &displayRequest, pdMS_TO_TICKS(10)) != pdTRUE) {
                ESP_LOGW(TAG, "Failed to send display request to queue");
            }

            if (!isFullRenderRequired) {
                partialRenderCount++;
            }
        }
    }
}

static void renderTask(void *pvParameters) {
    DisplayRequest displayRequest;

    for(;;) {
        if(xQueueReceive(displayQueue, &displayRequest, portMAX_DELAY) == pdPASS) {
            switch (displayRequest.paintType) {
                case DISPLAY_PAINT_TYPE_FULL:
                    renderToDisplay(&displayRequest.displayState);
                    break;
                case DISPLAY_PAINT_TYPE_PARTIAL:
                    partialRenderToDisplay(&displayRequest.displayState);
                    break;    
            }
        }
    }
}

static void serviceTask(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    float stateTemperatureC;
    float stateRelativeHumidity;
    TimeDate currentTime;

    for(;;) {
        enum env_error envStatus = readTemperatureAndHumidity(&stateTemperatureC, &stateRelativeHumidity);
        enum env_error timeStatus = getCurrentTime(&currentTime);
        enum env_error overallStatus = max(envStatus, timeStatus);
        int delayDuration = 15000; // Default to 15 seconds

        if (overallStatus == ENV_FAIL) {
            ESP_LOGE(TAG, "Failed to read environment data or time");
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(5000)); // Delay before retrying
            continue;
        } else if (overallStatus == ENV_WARNING) {
            delayDuration = 2000; // known stale state, try get fresh data sooner
            ESP_LOGW(TAG, "Environment data or time is in warning state");
        }

        AppEvent environmentEvent = {
            .eventType = APP_EVENT_ENVIRONMENT_UPDATED,
            .sensorUpdateData = {
                .temperatureC = stateTemperatureC,
                .relativeHumidity = stateRelativeHumidity
            },
            .timeUpdateData = currentTime
        };

        if(xQueueSend(appEventQueue, &environmentEvent, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to send environment event to queue");
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(delayDuration)); // Delay before the next reading
    }
}