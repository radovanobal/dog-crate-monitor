#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "../axpPower/axp_prot.h"

#include "button_pmu_bsp.h"

static const char *TAG = "button_pmu_bsp";

EventGroupHandle_t pmu_key_groups = NULL;


void buttonPmuBsp_init(void) {
    pmu_key_groups = xEventGroupCreate();

    if (pmu_key_groups == NULL) {
        ESP_LOGE(TAG, "Failed to create PMU key event group");
        return;
    }

    axp_enablePowerKeyIrq();
}


void buttonPmuBsp_poll(void) {
    AxpPowerKeyEvent powerEvent = axp_readPowerKeyEvent();

    if (powerEvent == AXP_POWER_KEY_EVENT_SHORT_PRESS) {
        xEventGroupSetBits(pmu_key_groups, PMU_KEY_BIT_SHORT_PRESS);
        return;
    } 
    
    if (powerEvent == AXP_POWER_KEY_EVENT_LONG_PRESS) {
        xEventGroupSetBits(pmu_key_groups, PMU_KEY_BIT_LONG_PRESS);
    }
}