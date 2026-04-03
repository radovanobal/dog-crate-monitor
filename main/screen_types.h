#ifndef DOG_CRATE_MONITOR_SCREEN_TYPES_H
#define DOG_CRATE_MONITOR_SCREEN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "./display_types.h"

typedef struct {
    DisplayRenderPlan displayRenderPlan;
} ScreenRenderResult;

typedef enum {
    SCREEN_ID_HOME = 0,
    SCREEN_ID_MENU = 1
} ScreenId;

typedef uint32_t ScreenGeneration;

typedef struct {
    int screenId;
    ScreenGeneration screenGeneration;
    DisplayRenderPlan displayRenderPlan;
} DisplayRequest;

typedef struct {
    DisplayRegionId regionId;
    bool isDirty;
} DirtyRegionEntry;

#endif // DOG_CRATE_MONITOR_SCREEN_TYPES_H