#ifndef DOC_CRATE_MONITOR_BUTTON_PMU_BSP_H
#define DOC_CRATE_MONITOR_BUTTON_PMU_BSP_H

#include "freertos/idf_additions.h"

extern EventGroupHandle_t pmu_key_groups;

#define PMU_KEY_BIT_SHORT_PRESS (1U << 0)
#define PMU_KEY_BIT_LONG_PRESS (1U << 1)
#define PMU_KEY_BIT_DOUBLE_CLICK (1U << 2)
#define PMU_KEY_BIT_ALL (PMU_KEY_BIT_SHORT_PRESS | PMU_KEY_BIT_LONG_PRESS | PMU_KEY_BIT_DOUBLE_CLICK)

void buttonPmuBsp_init(void);
void buttonPmuBsp_poll(void);

#endif // DOC_CRATE_MONITOR_BUTTON_PMU_BSP_H