#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bsp.h"

#include "./display.h"
#include "./environment.h"
#include "./task_manager.h"
#include "./app_dispatcher.h"
#include "./button_event.h"

// Log tag
static const char *TAG = "main";

static void initCommunications(void);


void app_main(void)
{
    initCommunications();
    initAppDispatcher();
    buttonEvent_init();

    if(initEnvironment() != ENV_SUCCESS) {
        return;
    }
    
    if (initDisplay() != DISPLAY_SUCCESS) {
        return;
    }

    if(initTaskManager() != TASK_MANAGER_SUCCESS) {
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

    if(xTaskCreatePinnedToCore(inputTask, "inputTask", 4096, NULL, 5, NULL, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Input task");
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