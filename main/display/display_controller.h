#ifndef DOG_CRATE_MONITOR_DISPLAY_CONTROLLER_H
#define DOG_CRATE_MONITOR_DISPLAY_CONTROLLER_H

#include "display_types.h"
#include "screen/screen_types.h"

display_init_error displayController_init(void);
void displayController_deinit(void);
void displayController_requestRender(const DisplayRequest *displayRequest);

#endif // DOG_CRATE_MONITOR_DISPLAY_CONTROLLER_H