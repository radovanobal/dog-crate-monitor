#ifndef DOG_CRATE_MONITOR_ENVIRONMENT_H
#define DOG_CRATE_MONITOR_ENVIRONMENT_H

#include <stddef.h>

enum env_error {
    ENV_SUCCESS = 0,
    ENV_WARNING = 1,
    ENV_FAIL = -1
};

enum env_error initEnvironment(void);
enum env_error readTemperatureAndHumidity(float *stateTemperatureC, float *stateRelativeHumidity);
void readClock(char *clockText, size_t clockTextSize);

#endif // DOG_CRATE_MONITOR_ENVIRONMENT_H