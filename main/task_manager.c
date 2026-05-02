#include <stdio.h>
#include <stdbool.h>

#include "display_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "freertos/queue.h"

#include "./task_manager.h"
#include "./utils.h"
#include "./environment_types.h"
#include "./environment.h"
#include "./app_event.h"
#include "./app_dispatcher.h"
#include "./button_event.h"
#include "./screen_manager.h"
#include "./power_management.h"
#include "./task_drain_monitor.h"
#include "./task_drain_events.h"

static void mergeDisplayRequests(DisplayRequest *accumulator, DisplayRequest *candidate);
static int findRegionInDisplayRequest(const DisplayRequest *request, DisplayRegionId regionId);

// Log tag
static const char *TAG = "task_manager";

static QueueHandle_t displayQueue = NULL;
static QueueHandle_t appEventQueue = NULL;

task_manager_error initTaskManager() {
    displayQueue = xQueueCreate(10, sizeof(DisplayRequest));

    if (displayQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create display queue");

        return TASK_MANAGER_FAIL;
    }

    taskDrainMonitor_setTaskQueueToMonitor(displayQueue);


    appEventQueue = xQueueCreate(10, sizeof(AppEvent));
    if (appEventQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create app event queue");
        vQueueDelete(displayQueue);
        displayQueue = NULL;

        return TASK_MANAGER_FAIL;
    }

    taskDrainMonitor_setTaskQueueToMonitor(appEventQueue);

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

        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_UI, TASK_DRAIN_STATE_DRAINING);

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

            taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_UI, TASK_DRAIN_STATE_IDLE);
            
            continue;
        }

        DisplayRequest displayRequest = screenManager_buildDisplayRequest(
            activeScreenId, 
            screenGeneration, 
            &renderResult
        );
        
        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_UI, TASK_DRAIN_STATE_IDLE);

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

        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_RENDER, TASK_DRAIN_STATE_DRAINING);

        DisplayRequest candidate;
        DisplayRequest accumulator = renderResult;
        
        for (;;) {
            if (xQueueReceive(displayQueue, &candidate, 0) != pdPASS) {
                ESP_LOGI(TAG, "No more display requests in queue to merge for this pass");

                break; // queue drained for this pass
            }

            if (candidate.screenGeneration < accumulator.screenGeneration) {
                // stale older work, ignore
                continue;
            }

            if (candidate.screenGeneration > accumulator.screenGeneration) {
                // newer generation invalidates old accumulated work
                accumulator = candidate;

                continue;
            }

            mergeDisplayRequests(&accumulator, &candidate);
        }

        const AppState *state = appDispatcher_getAppState();
        const ScreenGeneration screenGeneration = state->sharedState.navigationState.screenGeneration;

        if (accumulator.screenGeneration != screenGeneration) {
           ESP_LOGE(TAG,
                "BUG: stale render request generation %u received while active generation is %u",
                (unsigned)accumulator.screenGeneration,
                (unsigned)screenGeneration
            );

            taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_RENDER, TASK_DRAIN_STATE_IDLE);
            
            continue;
        }

        screenManager_render(&accumulator);
        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_RENDER, TASK_DRAIN_STATE_IDLE);
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

        if (buttonEvent.buttonType == BUTTON_EVENT_TYPE_POWER) {
            powerManagement_handlePowerButtonInput(buttonEvent);

            continue; // Power button events are handled separately, do not send to app event queue
        }

        powerManagement_abortPowerOff(); // any button event should abort power off if it was in progress

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
    int batteryLevel;
    TimeDate currentTime;

    ESP_LOGI(TAG, "Service Task started");

    for(;;) {
        if (powerManagement_isPreparingForPowerOff()) {
            ESP_LOGI(TAG, "System is preparing for power off, skipping service task operations");
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100)); // Delay before checking again

            continue;
        }

        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_SERVICE, TASK_DRAIN_STATE_DRAINING);

        enum env_error envStatus = readTemperatureAndHumidity(&stateTemperatureC, &stateRelativeHumidity);
        enum env_error timeStatus = getCurrentTime(&currentTime);
        enum env_error batteryStatus = getBatteryLevel(&batteryLevel);
        enum env_error overallStatus = max(max(envStatus, timeStatus), batteryStatus);
        int delayDuration = 15000; // Default to 15 seconds

        if (overallStatus == ENV_FAIL) {
            ESP_LOGE(TAG, "Failed to read environment data or time");
            taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_SERVICE, TASK_DRAIN_STATE_IDLE);
            
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(5000)); // Delay before retrying

            continue;
        }
        
        if (overallStatus == ENV_WARNING) {
            delayDuration = 2000; // known stale state, try get fresh data sooner
            ESP_LOGW(TAG, "Environment data or time is in warning state");
        }

        AppEvent environmentEvent = appEvent_createEnvironmentUpdateEvent(stateTemperatureC, stateRelativeHumidity, batteryLevel, currentTime);
        
        if(xQueueSend(appEventQueue, &environmentEvent, pdMS_TO_TICKS(10)) != pdTRUE) {
            ESP_LOGW(TAG, "Failed to send environment event to queue");
        }

        taskDrainMonitor_taskStatusChanged(TASK_DRAIN_TYPE_SERVICE, TASK_DRAIN_STATE_IDLE);

        if (powerManagement_isPreparingForPowerOff()) {
            ESP_LOGI(TAG, "System started preparing for power off during service task operations, skipping delay and checking power off state sooner");
            
            continue;
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(delayDuration)); // Delay before the next reading
    }
}

static void mergeDisplayRequests(DisplayRequest *accumulator, DisplayRequest *candidate) {
    if (candidate->screenGeneration != accumulator->screenGeneration) {
        ESP_LOGE(TAG,
            "BUG: Attempting to merge display requests of different generations: %u and %u",
            (unsigned)accumulator->screenGeneration,
            (unsigned)candidate->screenGeneration
        );

        return;
    }

    for (size_t i = 0; i < candidate->displayRenderPlan.count; i++) {
        const RenderRegionScene *candidateRegion = &candidate->displayRenderPlan.regions[i];
        const int accumulatorRegionIndex = findRegionInDisplayRequest(accumulator, candidateRegion->regionId);

        if (accumulatorRegionIndex < 0) {
            // region from candidate not in accumulator, add it
            if (accumulator->displayRenderPlan.count >= MAX_RENDER_SCENES) {
                ESP_LOGW(TAG, "Cannot merge display request: accumulator render plan is full");

                return;
            }
            
            accumulator->displayRenderPlan.regions[accumulator->displayRenderPlan.count++] = *candidateRegion;
            
            continue;
        }

        // region from candidate already in accumulator, replace it
        accumulator->displayRenderPlan.regions[accumulatorRegionIndex] = *candidateRegion;
    }
}

static int findRegionInDisplayRequest(const DisplayRequest *request, DisplayRegionId regionId) {
    for (size_t i = 0; i < request->displayRenderPlan.count; i++) {
        if (request->displayRenderPlan.regions[i].regionId == regionId) {
            return i;
        }
    }

    return -1;
}