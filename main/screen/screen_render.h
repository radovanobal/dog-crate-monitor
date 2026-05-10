#ifndef DOG_CRATE_MONITOR_SCREEN_RENDER_H
#define DOG_CRATE_MONITOR_SCREEN_RENDER_H 

#include "GUI_Paint.h"

#include "display/display_types.h"
#include "generated_icons.h"



PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, const char text[64], sFONT *font);
PixelRenderItem createTextUnderlineRenderItem(struct PixelCoordinates2D position, const char text[64], sFONT *font, DOT_PIXEL thickness);
PixelRenderItem createLineSeparatorRenderItem(struct PixelCoordinates2D start, struct PixelCoordinates2D end);
PixelRenderItem createBoxItem(struct PixelCoordinates2D position, struct PixelSize2D size, DOT_PIXEL thickness, DRAW_FILL fillType);
PixelRenderItem createIconBitmapRenderItem(struct PixelCoordinates2D position, IconId iconId, DisplayColor color);

#endif // DOG_CRATE_MONITOR_SCREEN_RENDER_H