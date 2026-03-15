#ifndef DOG_CRATE_MONITOR_ENVIRONMENT_H
#define DOG_CRATE_MONITOR_ENVIRONMENT_H

#include <stddef.h>

#include "./environment_types.h"

int getReadTryCount(void);
enum env_error initEnvironment(void);
enum env_error readTemperatureAndHumidity(float *stateTemperatureC, float *stateRelativeHumidity);
enum env_error getCurrentTime(TimeDate *currentTime);

#endif // DOG_CRATE_MONITOR_ENVIRONMENT_H