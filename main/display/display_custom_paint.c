#include <stdint.h>
#include <stdbool.h>

#include "GUI_Paint.h"
#include "esp_log.h"

#include "screen/screen_layout.h"
#include "display/display_custom_paint.h"
#include "display/display_types.h"
#include "generated_icons.h"

typedef struct {
    uint16_t mainColor;
    uint16_t backgroundColor;
    uint16_t foregroundColor;
    DRAW_FILL fillType;
} BoxGridCellStyle;

static const char *TAG = "display_custom_paint";

void paintIconToPixelBuffer(const IconBitmap *icon, const struct PixelCoordinates2D position, uint16_t color) {
    const uint16_t bytesPerRow = icon->bytesPerRow;

    for (uint16_t row = 0; row < icon->height; ++row) {
        for (uint16_t col = 0; col < icon->width; ++col) {
            const uint16_t byteIndex = row * bytesPerRow + col / 8;
            const uint8_t bitMask = (uint8_t)(0x80 >> (col % 8));
            const bool pixelOn = (icon->bitmap[byteIndex] & bitMask) != 0;

            if (pixelOn) {
                Paint_SetPixel(position.x + col, position.y + row, color);
            }
        }
    }
}

void paintBoxGrid(BoxGridData gridData) {
    const uint16_t cellWidth = gridData.size.width / gridData.columns;
    const uint16_t cellHeight = gridData.size.height / gridData.rows;
    const BoxGridCellStyle defaultCellStyle = {
        .mainColor = gridData.colorDefault,
        .backgroundColor = gridData.colorDefault,
        .foregroundColor = gridData.colorAccent,
        .fillType = DRAW_FILL_EMPTY
    };

    const BoxGridCellStyle activeCellStyle = {
        .mainColor = gridData.colorAccent,
        .backgroundColor = gridData.colorAccent,
        .foregroundColor = gridData.colorDefault,
        .fillType = DRAW_FILL_FULL
    };


    if (gridData.rows * gridData.columns > MAX_GRID_CELLS) {
        ESP_LOGE(TAG, "Grid cell count exceeds maximum supported cells. Cannot paint grid.");
        return;
    }

    for (uint8_t row = 0; row < gridData.rows; ++row) {
        for (uint8_t column = 0; column < gridData.columns; ++column) {
            const uint8_t cellIndex = row * gridData.columns + column;

            Paint_DrawRectangle(
                gridData.position.x + column * cellWidth,
                gridData.position.y + row * cellHeight,
                gridData.position.x + (column + 1) * cellWidth - 1,
                gridData.position.y + (row + 1) * cellHeight - 1,
                gridData.cells[cellIndex].isActive ? activeCellStyle.mainColor : defaultCellStyle.mainColor,
                DOT_PIXEL_1X1,
                gridData.cells[cellIndex].isActive ? activeCellStyle.fillType : defaultCellStyle.fillType
            );
        }
    }

    for (size_t i = 0; i < gridData.rows * gridData.columns && i < MAX_GRID_CELLS; ++i) {
        BoxGridCellData cellData = gridData.cells[i];
        int cellColumn = i % gridData.columns;
        int cellRow = i / gridData.columns;

        const uint16_t cellX = gridData.position.x + cellColumn * cellWidth;
        const uint16_t cellY = gridData.position.y + cellRow * cellHeight;

        struct PixelCoordinates2D textPosition = calculateAlignedTextPosition(
            &(DisplayRegionDescriptor){
                .pixelRegion = (PixelRegion){
                    .x = cellX,
                    .y = cellY,
                    .width = cellWidth * cellData.columnSpan,
                    .height = cellHeight
                }
            },
            cellData.text,
            &Font16,
            REGION_ALIGNMENT_CENTER
        );

        Paint_DrawString_EN(
            textPosition.x,
            textPosition.y,
            cellData.text,
            &Font16,
            gridData.cells[i].isActive ? activeCellStyle.foregroundColor : defaultCellStyle.foregroundColor,
            gridData.cells[i].isActive ? activeCellStyle.backgroundColor : defaultCellStyle.backgroundColor
        );
    }

}