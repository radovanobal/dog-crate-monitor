#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"

#include "esp_log.h"
#include "i2c_bsp.h"

#include "./environment.h"
#include "./task_manager.h"
#include "./app_dispatcher.h"
#include "./button_event.h"
#include "./display_controller.h"

// Log tag
static const char *TAG = "main";

static void initCommunications(void);

void app_main(void)
{
    initCommunications();
    
    const display_init_error err = displayController_init();
    
    if (err != DISPLAY_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize display controller with error code: %d", err);
        return;
    }
    
    appDispatcher_init();
    buttonEvent_init();

    if(initEnvironment() != ENV_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize environment");
        return;
    }

    if(initTaskManager() != TASK_MANAGER_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize task manager");
        return;
    }

    if(xTaskCreatePinnedToCore(uiTask, "uiTask", 8192, NULL, 5, NULL, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UI task");
        return;
    }   

    if(xTaskCreatePinnedToCore(renderTask, "renderTask", 8192, NULL, 4, NULL, 0) != pdPASS) {
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