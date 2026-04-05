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
#include "./display_controller.h"

// Log tag
static const char *TAG = "task_manager";

static QueueHandle_t displayQueue = NULL;
static QueueHandle_t appEventQueue = NULL;

task_manager_error initTaskManager() {
    displayQueue = xQueueCreate(10, sizeof(DisplayRequest));
    appEventQueue = xQueueCreate(10, sizeof(AppEvent));

    if (displayQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create display queue");
        return TASK_MANAGER_FAIL;
    }

    appEventQueue = xQueueCreate(10, sizeof(AppEvent));
    if (appEventQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create app event queue");
        vQueueDelete(displayQueue);
        displayQueue = NULL;
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
        const ScreenGeneration previousScreenGeneration = state->sharedState.navigationState.screenGeneration;

        ScreenActionResult actionResult = screenManager_handleEvent(&event, state);
        appDispatcher_dispatchEvent(&event);

        if (actionResult.screenIntent.intentType != SCREEN_INTENT_TYPE_NONE) {
            appDispatcher_applyScreenIntent(&actionResult.screenIntent);
        }

        const ScreenId activeScreenId = state->sharedState.navigationState.activeScreen;
        const ScreenGeneration screenGeneration = state->sharedState.navigationState.screenGeneration;

        if (screenGeneration != previousScreenGeneration) {
            ESP_LOGI(TAG,
                "Screen generation changed from %u to %u. Resetting display queue.",
                (unsigned)previousScreenGeneration,
                (unsigned)screenGeneration
            );
            xQueueReset(displayQueue);
        }

        ScreenRenderResult renderResult = screenManager_evaluateDisplay(state);

        if (renderResult.displayRenderPlan.count == 0) {
            ESP_LOGI(
                TAG, "Screen render not required after handling event of type: %d",
                event.eventType
            );
            continue;
        }

        DisplayRequest displayRequest = screenManager_buildDisplayRequest(
            activeScreenId, 
            screenGeneration, 
            &renderResult
        );

        if (xQueueSend(displayQueue, &displayRequest, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to enqueue display request");
            continue;
        }
    }
}

void renderTask(void *pvParameters) {
    ESP_LOGI(TAG, "Render Task started");
    DisplayRequest renderResult;

    for(;;) {
        if(xQueueReceive(displayQueue, &renderResult, portMAX_DELAY) != pdPASS) {
            ESP_LOGW(TAG, "Failed to receive display request from queue");
            continue;
        }
        
        const AppState *state = appDispatcher_getAppState();
        const ScreenGeneration screenGeneration = state->sharedState.navigationState.screenGeneration;

        if (renderResult.screenGeneration != screenGeneration) {
           ESP_LOGE(TAG,
                "BUG: stale render request generation %u received while active generation is %u",
                (unsigned)renderResult.screenGeneration,
                (unsigned)screenGeneration
            );
            continue;
        }

        screenManager_render(&renderResult);
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