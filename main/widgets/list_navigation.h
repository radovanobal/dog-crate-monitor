#ifndef DOG_CRATE_MONITOR_LIST_NAVIGATION_H
#define DOG_CRATE_MONITOR_LIST_NAVIGATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../display_types.h"
#include "../app_event.h"

typedef uint16_t ListItemId;

typedef struct {
    const char* label;
    sFONT *font;
    ListItemId id;
    bool isEnabled;
} ListNavigationItem;

typedef enum {
    LIST_NAVIGATION_ACTION_NONE = 0,
    LIST_NAVIGATION_ACTION_CANCELLED,
    LIST_NAVIGATION_ACTION_SELECTION_CHANGED,
    LIST_NAVIGATION_ACTION_SELECTION_CONFIRMED,
} ListNavigationActionType;

typedef struct {
    ListNavigationActionType actionType;
    ListItemId selectedItemId;
} ListNavigationResult;

void listNavigation_init(const ListNavigationItem *items, size_t itemCount, ListItemId activeItemId);
void listNavigation_deinit();
void listNavigation_buildRenderPlan(DisplayRenderPlan *renderPlan);
void listNavigation_setActiveItem(ListItemId activeItemId);
ListNavigationResult listNavigation_handleInput(InputEventData event);

#endif // DOG_CRATE_MONITOR_LIST_NAVIGATION_H