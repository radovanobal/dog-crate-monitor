#ifndef DOG_CRATE_MONITOR_SCREEN_RENDER_H
#define DOG_CRATE_MONITOR_SCREEN_RENDER_H 

#include "display_types.h"

PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, char text[16], sFONT *font);

#endif // DOG_CRATE_MONITOR_SCREEN_RENDER_H