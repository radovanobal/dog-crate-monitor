#include "screen_layout.h"
#include "display_types.h"
#include "./utils/macros.h"


ScreenLayout initRenderGrid(const struct GridConfig gridConfig)
{
    ScreenLayout layout = (ScreenLayout){
        .cellWidth = gridConfig.width / gridConfig.columns,
        .cellHeight = gridConfig.height / gridConfig.rows
    };

    return layout;
}

struct PixelCoordinates2D pixelRegionCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y + (pixelRegion.height - pixelItemSize.height) / 2;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

struct PixelCoordinates2D pixelRegionTopCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

struct PixelCoordinates2D pixelRegionTopRight(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + pixelRegion.width - pixelItemSize.width;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

struct PixelCoordinates2D pixelRegionTopLeft(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

PixelRegion regionToPixelSpace(struct GridRegion gridRegion, ScreenLayout screenLayout) {
    const int pixelX = gridRegion.x * screenLayout.cellWidth;
    const int pixelY = gridRegion.y * screenLayout.cellHeight;
    const int pixelWidth = gridRegion.width * screenLayout.cellWidth;
    const int pixelHeight = gridRegion.height * screenLayout.cellHeight;

    return (PixelRegion){ .x = pixelX, .y = pixelY, .width = pixelWidth, .height = pixelHeight };
}

void calculateDisplayRegionsPixelSpace(DisplayRegionDescriptor *displayRegions, size_t regionCount, ScreenLayout screenLayout) {
    for (size_t i = 0; i < regionCount; i++) {
        displayRegions[i].pixelRegion = regionToPixelSpace(displayRegions[i].gridRegion, screenLayout);
    }
}   

struct PixelCoordinates2D calculateAlignedTextPosition(
    DisplayRegionDescriptor *displayRegion,
    const char displayText[16], 
    const sFONT *font, 
    RegionAlignment alignment
) {
    const PixelRegion pixelRegion = displayRegion->pixelRegion;
    const struct PixelSize2D textBoxSize = (struct PixelSize2D){ .width = strlen(displayText) * font->Width, .height = font->Height };
    struct PixelCoordinates2D textPosition;

    switch (alignment) {
        case REGION_ALIGNMENT_CENTER:
            textPosition = pixelRegionCenter(pixelRegion, textBoxSize);
            break;
        case REGION_ALIGNMENT_TOP_CENTER:
            textPosition = pixelRegionTopCenter(pixelRegion, textBoxSize);
            break;
        case REGION_ALIGNMENT_TOP_RIGHT:
            textPosition = pixelRegionTopRight(pixelRegion, textBoxSize);
            break;
        case REGION_ALIGNMENT_TOP_LEFT:
            textPosition = pixelRegionTopLeft(pixelRegion, textBoxSize);
            break;
        default:
            textPosition = pixelRegionCenter(pixelRegion, textBoxSize);
            break;
    }

    return textPosition;
}
