#ifndef DOG_CRATE_MONITOR_SCREEN_RENDER_H
#define DOG_CRATE_MONITOR_SCREEN_RENDER_H 

#include "GUI_Paint.h"

#include "display_types.h"


PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, char text[16], sFONT *font);
PixelRenderItem createTextUnderlineRenderItem(struct PixelCoordinates2D position, char text[16], sFONT *font, DOT_PIXEL thickness);

#endif // DOG_CRATE_MONITOR_SCREEN_RENDER_H