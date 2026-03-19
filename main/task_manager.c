#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "./task_manager.h"
#include "./utils.h"
#include "./environment_types.h"
#include "./environment.h"
#include "./display.h"

static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate currentTime);

// Log tag
static const char *TAG = "task_manager";
static int maxPartialRenderCount = 100;
static QueueHandle_t displayQueue = NULL;
static QueueHandle_t appEventQueue = NULL;

task_manager_error initTaskManager() {
    displayQueue = xQueueCreate(10, sizeof(DisplayRequest));
    appEventQueue = xQueueCreate(10, sizeof(AppEvent));

    if (displayQueue == NULL || appEventQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create queues");
        return TASK_MANAGER_FAIL;
    }
    
    return TASK_MANAGER_SUCCESS;
}


void uiTask(void *pvParameters) {
    AppEvent event;
    DisplayState currentDisplayState = {0};
    int partialRenderCount = maxPartialRenderCount;
 
    ESP_LOGI(TAG, "UI Task started");

    for(;;) {
        if(xQueueReceive(appEventQueue, &event, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Received app event of type: %d", event.eventType);

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

void renderTask(void *pvParameters) {
    DisplayRequest displayRequest;

    ESP_LOGI(TAG, "Render Task started");

    for(;;) {
        if(xQueueReceive(displayQueue, &displayRequest, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Received display request with paint type: %s", 
                displayRequest.paintType == DISPLAY_PAINT_TYPE_FULL ? "FULL" : "PARTIAL"
            );

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

void serviceTask(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    float stateTemperatureC;
    float stateRelativeHumidity;
    TimeDate currentTime;

    ESP_LOGI(TAG, "Service Task started");

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
