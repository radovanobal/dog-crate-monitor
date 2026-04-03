#ifndef DOG_CRATE_MONITOR_SCREEN_LAYOUT_H
#define DOG_CRATE_MONITOR_SCREEN_LAYOUT_H

#include "display_types.h"

typedef struct {
    int cellWidth;
    int cellHeight;
} ScreenLayout;

typedef enum {
    REGION_ALIGNMENT_CENTER = 0,
    REGION_ALIGNMENT_TOP_CENTER = 1,
    REGION_ALIGNMENT_TOP_RIGHT = 2
} RegionAlignment;

ScreenLayout initRenderGrid(const struct GridConfig gridConfig);
struct PixelCoordinates2D pixelRegionCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize);
struct PixelCoordinates2D pixelRegionTopCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize);
struct PixelCoordinates2D pixelRegionTopRight(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize);
struct PixelCoordinates2D calculateAlignedTextPosition(DisplayRegionDescriptor *displayRegion, const char displayText[16], const sFONT *font, RegionAlignment alignment);
PixelRegion regionToPixelSpace(struct GridRegion gridRegion, ScreenLayout screenLayout);
void calculateDisplayRegionsPixelSpace(DisplayRegionDescriptor *displayRegions, size_t regionCount, ScreenLayout screenLayout);

#endif // DOG_CRATE_MONITOR_SCREEN_LAYOUT_H