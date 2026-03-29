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
#include "./app_event.h"
#include "./app_dispatcher.h"
#include "./button_event.h"
#include "./screen_manager.h"


// Log tag
static const char *TAG = "task_manager";
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
    ESP_LOGI(TAG, "UI Task started");

    for(;;) {
        AppEvent event;

        if(xQueueReceive(appEventQueue, &event, portMAX_DELAY) != pdPASS) {
            ESP_LOGW(TAG, "Failed to receive app event from queue");
            continue;
        }

        ESP_LOGI(TAG, "Received app event of type: %d", event.eventType);
        const AppState *state = appDispatcher_getAppState();

        ScreenActionResult actionResult = screenManager_handleEvent(&event, state);
        appDispatcher_dispatchEvent(&event);

        if (actionResult.screenIntent.intentType != SCREEN_INTENT_TYPE_NONE) {
            appDispatcher_applyScreenIntent(&actionResult.screenIntent);
        }

        ScreenRenderResult renderResult = screenManager_evaluateDisplay(state);

        if (!renderResult.isRenderRequired) {
            ESP_LOGI(
                TAG, "Screen render not required after handling event of type: %d",
                event.eventType
            );
            continue;
        }

        const ScreenId activeScreenId = state->sharedState.navigationState.activeScreen;
        DisplayRequest displayRequest = screenManager_buildDisplayRequest(activeScreenId, &renderResult.displayState);

        if(xQueueSend(displayQueue, &displayRequest, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to send display request to queue");
        } else {
            screenManager_setLastEnqueuedDisplayState(&displayRequest);
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

            const AppState *state = appDispatcher_getAppState();
            const ScreenId screenId = state->sharedState.navigationState.activeScreen;

            if (displayRequest.screenId != screenId) {
                ESP_LOGW(TAG, "Display request screen ID %d does not match active screen ID %d. Ignoring render request.",
                    displayRequest.screenId, screenId
                );
                continue;
            }

            screenManager_render(&displayRequest);
        }
    }
}

void inputTask(void *pvParameters) {
    ESP_LOGI(TAG, "Input Task started");

    for(;;) {
        ButtonEvent buttonEvent;

        if (!buttonEvent_wait(&buttonEvent, portMAX_DELAY)) {
            ESP_LOGW(TAG, "Failed to receive button event");
            continue;
        }

        AppEvent appEvent = {
            .eventType = APP_EVENT_INPUT_RECEIVED,
            .data.inputEventData = {
                .buttonType = buttonEvent.buttonType,
                .pressType = buttonEvent.pressType,
            }
        };

        if(xQueueSend(appEventQueue, &appEvent, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to send input event to queue");
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

        AppEvent environmentEvent = appEvent_createEnvironmentUpdateEvent(stateTemperatureC, stateRelativeHumidity, currentTime);
        
        if(xQueueSend(appEventQueue, &environmentEvent, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to send environment event to queue");
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(delayDuration)); // Delay before the next reading
    }
}