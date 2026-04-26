#include <stddef.h>
#include <stdint.h>

#include "epaper_port.h"
#include "GUI_Paint.h"

#include "./display_controller.h"
#include "./display_types.h"
#include "./display_pipeline_mono.h"
#include "./screen_types.h"
#include "./generated_icons.h"

typedef struct {
    RenderRegionScene scenes[MAX_RENDER_SCENES];
    size_t count;
} RenderSceneCache;

typedef struct {
    DisplayRegionId regionId;
    uint16_t regionPaintCount;
} RegionRefreshState;

typedef struct {
    const RenderRegionScene *scenes[MAX_RENDER_SCENES];
    size_t count;
} RegionRecoveryPlan;

typedef struct {
    RegionRefreshState states[MAX_RENDER_SCENES];
    size_t count;
} RegionRefreshStateCache;

static void initEpaperDisplay(void);
static void clearCachedScenePlan(void);
static void fullRenderToDisplay();
static void selectDisplayPipeline(DisplayPipelineType pipelineType);
static void partialRenderToDisplay(const DisplayRenderPlan *displayRenderPlan);
static void paintRenderPlan();
static void paintScene(const RenderRegionScene *scene);
static void paintSceneItem(const PixelRenderItem *item, const PixelRegion *pixelRegion);
static void updateRenderPlanCache(const DisplayRenderPlan *newPlan);
static bool didScreenGenerationChanged(ScreenGeneration current, ScreenGeneration last);
static bool doesScenePlanMatchCache(const DisplayRenderPlan *incomingPlan);
static bool doesRenderRegionSceneEquals(const RenderRegionScene *left, const RenderRegionScene *right);
static bool pixelRenderItemEquals(const PixelRenderItem *item1, const PixelRenderItem *item2);
static bool pixelRegionEquals(const PixelRegion *left, const PixelRegion *right);
static int findCachedSceneIndexByRegionId(DisplayRegionId regionId);
static DisplayPaintType determinePaintType(const DisplayRenderPlan *displayRenderPlan, ScreenGeneration screenGeneration);
static void paintIconToPixelBuffer(const IconBitmap *icon, const struct PixelCoordinates2D position, uint16_t color);

static const char *TAG = "display_controller";

static RenderSceneCache cacheRenderPlan = {0};
static ScreenGeneration lastRenderedScreenGeneration = 0;
static DisplayPipelineInterface *displayPipeline = NULL;

display_init_error displayController_init(void) {
    initEpaperDisplay();
    return DISPLAY_SUCCESS;
}

void displayController_deinit(void) {
    if (displayPipeline != NULL) {
        displayPipeline->deinit();
        displayPipeline = NULL;
    }
}

void displayController_requestRender(const DisplayRequest *displayRequest) {
    selectDisplayPipeline(displayRequest->pipelineType);

    if(didScreenGenerationChanged(displayRequest->screenGeneration, lastRenderedScreenGeneration)) {
        clearCachedScenePlan();
    }

    DisplayPaintType paintType = determinePaintType(&displayRequest->displayRenderPlan, displayRequest->screenGeneration);
    
    updateRenderPlanCache(&displayRequest->displayRenderPlan);

    switch (paintType) {
        case DISPLAY_PAINT_TYPE_FULL:
            displayPipeline->prepareBuffer(&displayRequest->displayRenderPlan, DISPLAY_PAINT_TYPE_FULL);
            fullRenderToDisplay();
            displayPipeline->flushBufferToDisplay(DISPLAY_PAINT_TYPE_FULL);
            break;
        case DISPLAY_PAINT_TYPE_PARTIAL:
            displayPipeline->prepareBuffer(&displayRequest->displayRenderPlan, DISPLAY_PAINT_TYPE_PARTIAL);
            partialRenderToDisplay(&displayRequest->displayRenderPlan);
            displayPipeline->flushBufferToDisplay(DISPLAY_PAINT_TYPE_PARTIAL);
            break;
        case DISPLAY_PAINT_TYPE_NONE:
            ESP_LOGI(TAG, "No paint required for screen generation %u", (unsigned)displayRequest->screenGeneration);
            break;    
        default:
            ESP_LOGW(TAG, "Unknown paint type: %d", paintType);
    }

    lastRenderedScreenGeneration = displayRequest->screenGeneration;
}

static void initEpaperDisplay(void) {
    ESP_LOGI(TAG,"1.e-Paper Init and Clear...");
    epaper_port_init();
}

static void selectDisplayPipeline(DisplayPipelineType pipelineType) {
    if (displayPipeline != NULL && displayPipeline->pipelineType == pipelineType) {
        return;
    }

    switch (pipelineType) {
        case DISPLAY_PIPELINE_TYPE_MONO:
            displayPipeline = displayPipelineMono_getPipelineInterface();
            break;
        case DISPLAY_PIPELINE_TYPE_GRAYSCALE:
            displayPipeline = displayPipelineMono_getPipelineInterface();
            // TODO: Implement grayscale pipeline and remove this log
            break;    
        default:
            ESP_LOGW(TAG, "Unknown display pipeline type: %d. Defaulting to mono pipeline.", pipelineType);
            displayPipeline = displayPipelineMono_getPipelineInterface();
    }

    if (displayPipeline->init() != DISPLAY_SUCCESS) {
        ESP_LOGE(TAG, "Failed to initialize display pipeline");
    }
}

static void updateRenderPlanCache(const DisplayRenderPlan *newPlan) {
    for (size_t i = 0; i < newPlan->count && i < MAX_RENDER_SCENES; i++) {
        const RenderRegionScene planScene = newPlan->regions[i];
        const int cachedIndex = findCachedSceneIndexByRegionId(planScene.regionId);

        if (cachedIndex >= 0) {
            cacheRenderPlan.scenes[cachedIndex] = planScene;
            continue;
        }

        if (cacheRenderPlan.count >= MAX_RENDER_SCENES) {
            ESP_LOGW(TAG, "Render scene cache is full. Cannot cache new scene with region ID: %d", planScene.regionId);
            return;
        }

        cacheRenderPlan.scenes[cacheRenderPlan.count++] = planScene;
    }
}

static void clearCachedScenePlan(void) {
    cacheRenderPlan = (RenderSceneCache){0};
}

static bool didScreenGenerationChanged(ScreenGeneration current, ScreenGeneration last) {
    return current != last;
}

static DisplayPaintType determinePaintType(const DisplayRenderPlan *displayRenderPlan, ScreenGeneration screenGeneration) {
    if (didScreenGenerationChanged(screenGeneration, lastRenderedScreenGeneration)) {
        ESP_LOGI(TAG, "Screen generation changed from %u to %u. Forcing full render.",
            (unsigned)lastRenderedScreenGeneration,
            (unsigned)screenGeneration
        );

        return DISPLAY_PAINT_TYPE_FULL;
    }
    
    if (doesScenePlanMatchCache(displayRenderPlan)) {
        ESP_LOGI(TAG, "Display render plan matches cache. No paint required.");
        return DISPLAY_PAINT_TYPE_NONE;
    }

    if (displayRenderPlan->count == 0) {
        ESP_LOGW(TAG, "No items to render in the display render plan");
        return DISPLAY_PAINT_TYPE_NONE;
    }

    return DISPLAY_PAINT_TYPE_PARTIAL;
}

static bool doesScenePlanMatchCache(const DisplayRenderPlan *incomingPlan) {
    for (size_t i = 0; i < incomingPlan->count; i++) {
        const RenderRegionScene *incomingScene = &incomingPlan->regions[i];
        const int cachedIndex = findCachedSceneIndexByRegionId(incomingScene->regionId);

        if (cachedIndex < 0) {
            return false;
        }

        if (!doesRenderRegionSceneEquals(incomingScene, &cacheRenderPlan.scenes[cachedIndex])) {
            return false;
        }
    }

    return true;
}

static bool doesRenderRegionSceneEquals(const RenderRegionScene *left, const RenderRegionScene *right) {
    if (left->regionId != right->regionId) {
        return false;
    }

    if (!pixelRegionEquals(&left->pixelRegion, &right->pixelRegion)) {
        return false;
    }

    if (left->count != right->count) {
        return false;
    }

    for (size_t i = 0; i < left->count; i++) {
        if (!pixelRenderItemEquals(&left->renderItems[i], &right->renderItems[i])) {
            return false;
        }
    }

    return true;
}

static int findCachedSceneIndexByRegionId(DisplayRegionId regionId) {
    for (size_t i = 0; i < cacheRenderPlan.count; i++) {
        if (cacheRenderPlan.scenes[i].regionId == regionId) {
            return i;
        }
    }

    return -1;
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

    switch (left->type) {
        case RENDER_ITEM_TYPE_CLEAR:
            return true;

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
        case RENDER_ITEM_TYPE_ICON:
            return left->data.icon.position.x == right->data.icon.position.x &&
                   left->data.icon.position.y == right->data.icon.position.y &&
                   left->data.icon.iconId == right->data.icon.iconId;
    }


    return false;
}

static void paintRenderPlan() {
    for (size_t i = 0; i < cacheRenderPlan.count; i++) {
        const RenderRegionScene *scene = &cacheRenderPlan.scenes[i];

        paintScene(scene);
    }
}

static void paintScene(const RenderRegionScene *scene) {
    for (size_t i = 0; i < scene->count; i++) {
        const PixelRenderItem *item = &scene->renderItems[i];

        paintSceneItem(item, &scene->pixelRegion);
    }
}

static void clearSceneRegion(const PixelRegion *pixelRegion) {
    PixelRenderItem clearItem = {
        .type = RENDER_ITEM_TYPE_CLEAR,
    };

    paintSceneItem(&clearItem, pixelRegion);
}

static void paintSceneItem(const PixelRenderItem *item, const PixelRegion *pixelRegion) {
    switch (item->type) {
        case RENDER_ITEM_TYPE_CLEAR:
            Paint_DrawRectangle(
                pixelRegion->x,
                pixelRegion->y,
                pixelRegion->x + pixelRegion->width - 1,
                pixelRegion->y + pixelRegion->height - 1,
                WHITE,
                DOT_PIXEL_1X1, 
                DRAW_FILL_FULL
            );
            break;
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
        case RENDER_ITEM_TYPE_ICON:
            paintIconToPixelBuffer(findIconBitmap(item->data.icon.iconId), item->data.icon.position, BLACK);
            break;
        default:
            ESP_LOGW(TAG, "Unknown render item type: %d", item->type);
    }

}

static void paintIconToPixelBuffer(const IconBitmap *icon, const struct PixelCoordinates2D position, uint16_t color) {
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

static void fullRenderToDisplay(void) {
    paintRenderPlan();
}

static void partialRenderToDisplay(const DisplayRenderPlan *displayRenderPlan) {
    if(displayRenderPlan->count == 0) {
        ESP_LOGW(TAG, "No items to render in the display render plan");
        return;
    }

    for (size_t i = 0; i < displayRenderPlan->count; i++) {
        const RenderRegionScene *scene = &displayRenderPlan->regions[i];

        clearSceneRegion(&scene->pixelRegion);
        paintScene(scene);
    }
}