#include <stdint.h>

#include "epaper_port.h"
#include "epaper_bsp.h"
#include "GUI_Paint.h"

#include "./display_controller.h"
#include "./display_types.h"
#include "./screen_types.h"

static void initEpaperDisplay(void);
static void fullRenderToDisplay(const DisplayRenderPlan *displayRenderPlan);
static void partialRenderToDisplay(const DisplayRenderPlan *displayRenderPlan);
static void paintItems(const DisplayRenderPlan *displayRenderPlan);
static void paintPartialItems(const DisplayRenderPlan *displayRenderPlan);
static void paintItem(const PixelRenderItem *item);
static bool DisplayRenderPlanEquals(const DisplayRenderPlan *plan1, const DisplayRenderPlan *plan2);
static bool pixelRenderItemEquals(const PixelRenderItem *item1, const PixelRenderItem *item2);
static bool pixelRegionEquals(const PixelRegion *left, const PixelRegion *right);
static DisplayPaintType determinePaintType(ScreenGeneration screenGeneration, DisplayRenderPlan displayRenderPlan);

static const char *TAG = "display_controller";

static DisplayRenderPlan lastRenderPlan = {0};
static uint8_t *ImageMonoBuffer;
static const int maxPartialRenderCount = 100;
static int partialRenderCount = maxPartialRenderCount;
static ScreenGeneration lastRenderedScreenGeneration = 0;

display_init_error displayController_init(void) {
    initEpaperDisplay();

    if((ImageMonoBuffer = (UBYTE *)malloc(EPD_SIZE_MONO)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return DISPLAY_FAIL;
    }

    return DISPLAY_SUCCESS;
}

void displayController_deinit(void) {
    if (ImageMonoBuffer != NULL) {
        free(ImageMonoBuffer);
        ImageMonoBuffer = NULL;
    }
}

void displayController_requestRender(const DisplayRenderPlan *displayRenderPlan, ScreenGeneration screenGeneration) {
    DisplayPaintType paintType = determinePaintType(screenGeneration, *displayRenderPlan);

    switch (paintType) {
        case DISPLAY_PAINT_TYPE_FULL:
            partialRenderCount = 0;
            fullRenderToDisplay(displayRenderPlan);
            break;
        case DISPLAY_PAINT_TYPE_PARTIAL:
            partialRenderCount++;
            partialRenderToDisplay(displayRenderPlan);
            break;
        case DISPLAY_PAINT_TYPE_NONE:
            ESP_LOGI(TAG, "No paint required for screen generation %u", (unsigned)screenGeneration);
            break;    
        default:
            ESP_LOGW(TAG, "Unknown paint type: %d", paintType);
    }

    lastRenderPlan = *displayRenderPlan;
    lastRenderedScreenGeneration = screenGeneration;
}

static DisplayPaintType determinePaintType(ScreenGeneration screenGeneration, DisplayRenderPlan displayRenderPlan) {
    if (displayRenderPlan.count == 0) {
        ESP_LOGW(TAG, "No items to render in the display render plan");
        return DISPLAY_PAINT_TYPE_NONE;
    }

    if (DisplayRenderPlanEquals(&displayRenderPlan, &lastRenderPlan)) {
        ESP_LOGI(TAG, "Display render plan unchanged. No render required.");
        return DISPLAY_PAINT_TYPE_NONE;
    }

    if (screenGeneration != lastRenderedScreenGeneration) {
        ESP_LOGI(TAG, "Screen generation changed from %u to %u. Forcing full render.",
            (unsigned)lastRenderedScreenGeneration,
            (unsigned)screenGeneration
        );
        return DISPLAY_PAINT_TYPE_FULL;
    }
    
    if (partialRenderCount >= maxPartialRenderCount) {
        ESP_LOGI(TAG, "Max partial render count reached. Forcing full render.");
        return DISPLAY_PAINT_TYPE_FULL;
    } 

    return DISPLAY_PAINT_TYPE_PARTIAL;
}

static bool DisplayRenderPlanEquals(const DisplayRenderPlan *plan1, const DisplayRenderPlan *plan2) {
    if (plan1->count != plan2->count) {
        return false;
    }

    for (size_t i = 0; i < plan1->count; i++) {
        const PixelRenderItem *item1 = &plan1->items[i];
        const PixelRenderItem *item2 = &plan2->items[i];

        if (!pixelRenderItemEquals(item1, item2)) {
            return false;
        }
    }

    return true;
}

static bool pixelRegionEquals(const PixelRegion *left, const PixelRegion *right) {
    return left->x == right->x &&
           left->y == right->y &&
           left->width == right->width &&
           left->height == right->height;
}


static bool pixelRenderItemEquals(const PixelRenderItem *left, const PixelRenderItem *right) {
    if (left->type != right->type) {
        return false;
    }

    if (!pixelRegionEquals(&left->pixelRegion, &right->pixelRegion)) {
        return false;
    }

    switch (left->type) {
        case RENDER_ITEM_TYPE_TEXT:
            return left->data.text.position.x == right->data.text.position.x &&
                   left->data.text.position.y == right->data.text.position.y &&
                   left->data.text.font == right->data.text.font &&
                   strcmp(left->data.text.text, right->data.text.text) == 0;

        case RENDER_ITEM_TYPE_BITMAP:
            return left->data.bitmap.position.x == right->data.bitmap.position.x &&
                   left->data.bitmap.position.y == right->data.bitmap.position.y &&
                   left->data.bitmap.imageData == right->data.bitmap.imageData &&
                   left->data.bitmap.size.width == right->data.bitmap.size.width &&
                   left->data.bitmap.size.height == right->data.bitmap.size.height;

        case RENDER_ITEM_TYPE_RECT:
            return left->data.rect.position.x == right->data.rect.position.x &&
                   left->data.rect.position.y == right->data.rect.position.y &&
                   left->data.rect.size.width == right->data.rect.size.width &&
                   left->data.rect.size.height == right->data.rect.size.height &&
                   left->data.rect.color == right->data.rect.color &&
                   left->data.rect.thickness == right->data.rect.thickness &&
                   left->data.rect.fillType == right->data.rect.fillType;

        case RENDER_ITEM_TYPE_LINE:
            return left->data.line.start.x == right->data.line.start.x &&
                   left->data.line.start.y == right->data.line.start.y &&
                   left->data.line.end.x == right->data.line.end.x &&
                   left->data.line.end.y == right->data.line.end.y &&
                   left->data.line.color == right->data.line.color &&
                   left->data.line.thickness == right->data.line.thickness &&
                   left->data.line.style == right->data.line.style;
    }

    return false;
}

static void initEpaperDisplay(void) {
    ESP_LOGI(TAG,"1.e-Paper Init and Clear...");
    epaper_port_init();
    EPD_Init();
    EPD_Clear();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

static void paintItems(const DisplayRenderPlan *displayRenderPlan) {
    for (size_t i = 0; i < displayRenderPlan->count; i++) {
        const PixelRenderItem *item = &displayRenderPlan->items[i];

        paintItem(item);
    }
}

static void paintPartialItems(const DisplayRenderPlan *displayRenderPlan) {
    for (size_t i = 0; i < displayRenderPlan->count; i++) {
        const PixelRenderItem *item = &displayRenderPlan->items[i];

        Paint_DrawRectangle(
            item->pixelRegion.x,
            item->pixelRegion.y,
            item->pixelRegion.x + item->pixelRegion.width - 1,
            item->pixelRegion.y + item->pixelRegion.height - 1,
            WHITE, 
            DOT_PIXEL_1X1, 
            DRAW_FILL_FULL
        );

        paintItem(item);

    }
}

static void paintItem(const PixelRenderItem *item) {
    switch (item->type) {
        case RENDER_ITEM_TYPE_TEXT:
            Paint_DrawString_EN(
                item->data.text.position.x,
                item->data.text.position.y,
                item->data.text.text, 
                item->data.text.font, 
                WHITE, 
                BLACK
            );
            break;
        case RENDER_ITEM_TYPE_BITMAP:
            Paint_ReadBmp(
                item->data.bitmap.imageData,
                item->data.bitmap.position.x,
                item->data.bitmap.position.y,
                item->data.bitmap.size.width,
                item->data.bitmap.size.height
            );
            break;
        case RENDER_ITEM_TYPE_RECT:
            Paint_DrawRectangle(
                item->data.rect.position.x,
                item->data.rect.position.y,
                item->data.rect.position.x + item->data.rect.size.width - 1,
                item->data.rect.position.y + item->data.rect.size.height - 1,
                item->data.rect.color,
                item->data.rect.thickness,
                item->data.rect.fillType
            );
            break;
        case RENDER_ITEM_TYPE_LINE:
            Paint_DrawLine(
                item->data.line.start.x,
                item->data.line.start.y,
                item->data.line.end.x,
                item->data.line.end.y,
                item->data.line.color, 
                item->data.line.thickness,
                item->data.line.style
            );
            break;
        default:
            ESP_LOGW(TAG, "Unknown render item type: %d", item->type);
    }

}

static void fullRenderToDisplay(const DisplayRenderPlan *displayRenderPlan) {
    Paint_NewImage(ImageMonoBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(ImageMonoBuffer);
    Paint_Clear(WHITE);

    paintItems(displayRenderPlan);

    EPD_Display_Base(ImageMonoBuffer);
}

static void partialRenderToDisplay(const DisplayRenderPlan *displayRenderPlan) {
    if(displayRenderPlan->count == 0) {
        ESP_LOGW(TAG, "No items to render in the display render plan");
        return;
    }

    Paint_NewImage(ImageMonoBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(ImageMonoBuffer);

    paintPartialItems(displayRenderPlan);

    /*
    When the driver supports partial update, use the code below to update the corresponding region of the screen.
    For now we have to target the full screen for partial updates, as the driver doesn't support region updates 
    and we need to sync the full buffer with the region image before updating the screen.

    EPD_Display_Partial(
        ImagePartialMono,
        pixelRegion.x,
        gridConfig.height - (pixelRegion.y + pixelRegion.height),
        pixelRegion.x + pixelRegion.width,
        gridConfig.height - pixelRegion.y
    );    

    */
    
    EPD_Display_Partial(
        ImageMonoBuffer,
        0,
        0,
        EPD_WIDTH,
        EPD_HEIGHT
    );
}