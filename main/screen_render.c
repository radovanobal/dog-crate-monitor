#include "screen_render.h"
#include "display_types.h"

PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, char text[16], sFONT *font) {
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