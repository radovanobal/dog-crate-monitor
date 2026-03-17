#ifndef DOG_CRATE_MONITOR_DISPLAY_H
#define DOG_CRATE_MONITOR_DISPLAY_H

#include "display_types.h"
#include "environment_types.h"

enum display_error {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
};

enum display_error initDisplay(void);
void renderToDisplay(DisplayState *displayState);
void partialRenderToDisplay(DisplayState *displayState);

#endif // DOG_CRATE_MONITOR_DISPLAY_H
