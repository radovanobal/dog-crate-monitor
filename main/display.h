#ifndef DOG_CRATE_MONITOR_DISPLAY_H
#define DOG_CRATE_MONITOR_DISPLAY_H

enum display_error {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
};

enum display_error initDisplay(void);
void renderToDisplay(float stateTemperatureC, float stateRelativeHumidity, const char *clockText);

#endif // DOG_CRATE_MONITOR_DISPLAY_H
