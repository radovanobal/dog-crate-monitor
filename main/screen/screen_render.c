#include "GUI_Paint.h"


#include "display/display_types.h"
#include "screen_render.h"
#include "generated_icons.h"

PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, const char text[16], sFONT *font) {
    PixelRenderItem renderItem = (PixelRenderItem){
        .type = RENDER_ITEM_TYPE_TEXT,
        .data = {
            .text = {
                .position = position,
                .font = font,
            }
        }
    };

    strncpy(renderItem.data.text.text, text, sizeof(renderItem.data.text.text) - 1);

    return renderItem;
}

PixelRenderItem createTextUnderlineRenderItem(struct PixelCoordinates2D position, const char text[16], sFONT *font, DOT_PIXEL thickness) {
    PixelRenderItem renderItem = (PixelRenderItem) {
        .type = RENDER_ITEM_TYPE_LINE,
        .data = {
            .line = {
                .start = (struct PixelCoordinates2D){
                    .x = position.x,
                    .y = position.y + font->Height + 2
                },
                .end = (struct PixelCoordinates2D){
                    .x = position.x + font->Width * strlen(text),
                    .y = position.y + font->Height + 2
                },
                .color = DISPLAY_COLOR_BLACK, // Black
                .thickness = thickness,
                .style = LINE_STYLE_SOLID  
            }
        }
    };
    return renderItem;
}

PixelRenderItem createLineSeparatorRenderItem(struct PixelCoordinates2D start, struct PixelCoordinates2D end) {
    PixelRenderItem renderItem = (PixelRenderItem) {
        .type = RENDER_ITEM_TYPE_LINE,
        .data = {
            .line = {
                .start = start,
                .end = end,
                .color = DISPLAY_COLOR_GRAY1, // Gray
                .thickness = DOT_PIXEL_1X1,
                .style = LINE_STYLE_SOLID  
            }
        }
    };
    return renderItem;
}

PixelRenderItem createIconBitmapRenderItem(struct PixelCoordinates2D position, IconId iconId, DisplayColor color) {
    PixelRenderItem renderItem = (PixelRenderItem) {
        .type = RENDER_ITEM_TYPE_ICON,
        .data = {
            .icon = {
                .position = position,
                .iconId = iconId,
                .color = color
            }
        }
    };
    return renderItem;
}
