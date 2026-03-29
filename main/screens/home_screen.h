#ifndef DOG_CRATE_MONITOR_DISPLAY_H
#define DOG_CRATE_MONITOR_DISPLAY_H

#include "../display_types.h"
#include "../environment_types.h"
#include "../app_event.h"
#include "../app_store.h"
#include "../screen_manager.h"

enum display_error {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
};

const ScreenInterface *homeScreen_getScreenInterface(void);

#endif // DOG_CRATE_MONITOR_DISPLAY_H
