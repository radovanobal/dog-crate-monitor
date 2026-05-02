#ifndef DOG_CRATE_MONITOR_POWER_MANAGEMENT_H
#define DOG_CRATE_MONITOR_POWER_MANAGEMENT_H

#include "button_event.h"
#include <stdbool.h>

void powerManagement_init(void);
void powerManagement_handlePowerButtonInput(ButtonEvent buttonEvent);
void powerManagement_abortPowerOff(void);
bool powerManagement_isPreparingForPowerOff(void);

#endif // DOG_CRATE_MONITOR_POWER_MANAGEMENT_H