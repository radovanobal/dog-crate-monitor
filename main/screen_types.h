#ifndef DOG_CRATE_MONITOR_SCREEN_TYPES_H
#define DOG_CRATE_MONITOR_SCREEN_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "./display_types.h"

typedef uint32_t ScreenGeneration;

typedef enum {
    SCREEN_INTENT_TYPE_NONE = 0,
    SCREEN_INTENT_TYPE_SCREEN_CHANGE = 1,
} ScreenIntentType;

typedef enum {
    SCREEN_ID_HOME = 0,
    SCREEN_ID_MENU = 1,
    SCREEN_ID_SETTINGS = 2,
} ScreenId;

typedef struct {
    int screenId;
    ScreenGeneration screenGeneration;
    DisplayRenderPlan displayRenderPlan;
} DisplayRequest;

typedef struct {
    DisplayRegionId regionId;
    bool isDirty;
} DirtyRegionEntry;

typedef struct {
    DisplayRenderPlan displayRenderPlan;
} ScreenRenderResult;

#endif // DOG_CRATE_MONITOR_SCREEN_TYPES_H