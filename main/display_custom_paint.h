#ifndef DOG_CRATE_MONITOR_DISPLAY_CUSTOM_PAINT_H
#define DOG_CRATE_MONITOR_DISPLAY_CUSTOM_PAINT_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/_intsup.h>

#include "generated_icons.h"
#include "display_types.h"


void paintIconToPixelBuffer(const IconBitmap *icon, const struct PixelCoordinates2D position, uint16_t color);
void paintBoxGrid(BoxGridData gridData);


#endif // DOG_CRATE_MONITOR_DISPLAY_CUSTOM_PAINT_H

