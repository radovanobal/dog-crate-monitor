#ifndef DOG_CRATE_MONITOR_ENVIRONMENT_TYPES_H
#define DOG_CRATE_MONITOR_ENVIRONMENT_TYPES_H

#include <stdint.h>

enum env_error {
    ENV_SUCCESS = 0,
    ENV_WARNING = 1,
    ENV_FAIL = 2
};

typedef struct{
    uint16_t years;
    uint16_t months;
    uint16_t days;
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t week;
}TimeDate;

#endif //DOG_CRATE_MONITOR_ENVIRONMENT_TYPES_H