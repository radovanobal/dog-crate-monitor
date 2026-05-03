#ifndef DOG_CRATE_MONITOR_POWER_MANAGEMENT_H
#define DOG_CRATE_MONITOR_POWER_MANAGEMENT_H

#include "button_event.h"
#include <stdbool.h>

void powerManagement_init(void);
void powerManagement_handlePowerButtonInput(ButtonEvent buttonEvent);
void powerManagement_abortTaskSuspend(void);
bool powerManagement_isPreparingForTaskSuspension(void);

#endif // DOG_CRATE_MONITOR_POWER_MANAGEMENT_H