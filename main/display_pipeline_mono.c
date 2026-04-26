#include "epaper_port.h"

#include "display_pipeline_mono.h"
#include "display_types.h"

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

static const char *TAG = "display_pipeline_mono";

static uint8_t *ImageMonoBuffer;
static RegionRefreshStateCache regionRefreshStateCache = {0};
static const int maxPartialRenderCount = 50;

static display_init_error init(void);
static void deinit(void);
static void prepareBufferFull(void);
static void prepareBufferPartial(const DisplayRenderPlan *displayRenderPlan);
static void flushBufferFull(void);
static void flushBufferPartial(void);
static void prepareBuffer(const DisplayRenderPlan *displayRenderPlan, const DisplayPaintType displayPaintType);
static void flushBufferToDisplay(const DisplayPaintType displayPaintType);
static int renderRegionPaintCountIncrement(const RenderRegionScene *scene);
static int findRenderCountSceneIndexByRegionId(DisplayRegionId regionId);
static RegionRecoveryPlan determineRegionsForRecovery(const DisplayRenderPlan *displayRenderPlan);
static void recoverRegions(const RegionRecoveryPlan *recoveryPlan);

DisplayPipelineInterface* displayPipelineMono_getPipelineInterface() {
    static const DisplayPipelineInterface pipelineInterface = {
        .init = init,
        .deinit = deinit,
        .prepareBuffer = prepareBuffer,
        .flushBufferToDisplay = flushBufferToDisplay,
        .pipelineType = DISPLAY_PIPELINE_TYPE_MONO
    };

    return (DisplayPipelineInterface*)&pipelineInterface;
};

static display_init_error init(void) {
    EPD_Init_Fast();
    EPD_Clear();   // No initialization needed for the mono pipeline at this time


    if((ImageMonoBuffer = (UBYTE *)malloc(EPD_SIZE_MONO)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return DISPLAY_FAIL;
    }

    return DISPLAY_SUCCESS;
}

static void deinit(void) {
    if (ImageMonoBuffer != NULL) {
        free(ImageMonoBuffer);
        ImageMonoBuffer = NULL;
    }
}

static void prepareBuffer(const DisplayRenderPlan *displayRenderPlan, const DisplayPaintType displayPaintType) {
    switch (displayPaintType) {
        case DISPLAY_PAINT_TYPE_FULL:
            prepareBufferFull();
            break;
        case DISPLAY_PAINT_TYPE_PARTIAL:
            prepareBufferPartial(displayRenderPlan);
            break;
        case DISPLAY_PAINT_TYPE_NONE:
            // No preparation needed
            break;
        default:
            ESP_LOGW(TAG, "Unknown paint type: %d", displayPaintType);
    }
}

static void flushBufferToDisplay(const DisplayPaintType displayPaintType) {
    switch (displayPaintType) {
        case DISPLAY_PAINT_TYPE_FULL:
            flushBufferFull();
            return;
        case DISPLAY_PAINT_TYPE_PARTIAL:
            flushBufferPartial();
            break;
        case DISPLAY_PAINT_TYPE_NONE:
            ESP_LOGI(TAG, "No flush required for paint type: %d", displayPaintType);
            return;
        default:
            ESP_LOGW(TAG, "Unknown paint type: %d", displayPaintType);
            return;
    }
}

static void prepareBufferFull(void) {
    Paint_NewImage(ImageMonoBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SetScale(2);
    Paint_SelectImage(ImageMonoBuffer);
    Paint_Clear(WHITE);
}

static void prepareBufferPartial(const DisplayRenderPlan *displayRenderPlan) {
    Paint_NewImage(ImageMonoBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(ImageMonoBuffer);

    RegionRecoveryPlan recoveryPlan = determineRegionsForRecovery(displayRenderPlan);
    if (recoveryPlan.count > 0) {
        ESP_LOGI(TAG, "Recovering %u regions before partial render", (unsigned)recoveryPlan.count);
        recoverRegions(&recoveryPlan);
    }
}

static void flushBufferFull() {
    EPD_Display_Base(ImageMonoBuffer);
    regionRefreshStateCache = (RegionRefreshStateCache){0};
}

static void flushBufferPartial(void) {
    EPD_Display_Partial(
        ImageMonoBuffer,
        0,
        0,
        EPD_WIDTH,
        EPD_HEIGHT
    );
}

static RegionRecoveryPlan determineRegionsForRecovery(const DisplayRenderPlan *displayRenderPlan) {
    RegionRecoveryPlan recoveryPlan = {0};
    
    for (size_t i = 0; i < displayRenderPlan->count; i++) {
        const RenderRegionScene *scene = &displayRenderPlan->regions[i];
        const int regionStateIndex = renderRegionPaintCountIncrement(scene);

        if (recoveryPlan.count >= MAX_RENDER_SCENES) {
            ESP_LOGW(TAG,
                "Recovery plan full (%u). Breaking recovery plan generation.",
                (unsigned)MAX_RENDER_SCENES
            );

            break;
        }

        if (regionStateIndex >= 0 && regionRefreshStateCache.states[regionStateIndex].regionPaintCount >= maxPartialRenderCount) {
            ESP_LOGI(TAG, "Region ID %d has been painted %u times. Marking for full refresh.",
                regionRefreshStateCache.states[regionStateIndex].regionId,
                (unsigned)regionRefreshStateCache.states[regionStateIndex].regionPaintCount
            );

            recoveryPlan.scenes[recoveryPlan.count++] = scene;
            regionRefreshStateCache.states[regionStateIndex].regionPaintCount = 0;
        }
    }

    return recoveryPlan;
}

static void recoverRegions(const RegionRecoveryPlan *recoveryPlan) {
    Paint_NewImage(ImageMonoBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(ImageMonoBuffer);

    for (size_t i = 0; i < recoveryPlan->count; i++) {
        const RenderRegionScene *scene = recoveryPlan->scenes[i];
        const PixelRegion *pixelRegion = &scene->pixelRegion;

        Paint_DrawRectangle(
            pixelRegion->x,
            pixelRegion->y,
            pixelRegion->x + pixelRegion->width - 1,
            pixelRegion->y + pixelRegion->height - 1,
            WHITE,
            DOT_PIXEL_1X1, 
            DRAW_FILL_FULL
        );
    }

    EPD_Display_Partial(
        ImageMonoBuffer,
        0,
        0,
        EPD_WIDTH,
        EPD_HEIGHT
    );

    for (size_t i = 0; i < recoveryPlan->count; i++) {
        const RenderRegionScene *scene = recoveryPlan->scenes[i];
        const PixelRegion *pixelRegion = &scene->pixelRegion;

        Paint_DrawRectangle(
            pixelRegion->x,
            pixelRegion->y,
            pixelRegion->x + pixelRegion->width - 1,
            pixelRegion->y + pixelRegion->height - 1,
            BLACK,
            DOT_PIXEL_1X1, 
            DRAW_FILL_FULL
        );
    }

    EPD_Display_Partial(
        ImageMonoBuffer,
        0,
        0,
        EPD_WIDTH,
        EPD_HEIGHT
    );
}

static int renderRegionPaintCountIncrement(const RenderRegionScene *scene) {
    const int regionStateIndex = findRenderCountSceneIndexByRegionId(scene->regionId);

    if (regionStateIndex >= 0) {
        regionRefreshStateCache.states[regionStateIndex].regionPaintCount++;
        return regionStateIndex;
    }

    if (regionRefreshStateCache.count >= MAX_RENDER_SCENES) {
        ESP_LOGW(
            TAG,
            "Region refresh state cache is full (%u). Skipping paint count tracking for region %u.",
            (unsigned)MAX_RENDER_SCENES,
            (unsigned)scene->regionId
        );
        return -1;
    }

    regionRefreshStateCache.states[regionRefreshStateCache.count++] = (RegionRefreshState){ 
        .regionId = scene->regionId, 
        .regionPaintCount = 1 
    };

    return regionRefreshStateCache.count - 1;
}

static int findRenderCountSceneIndexByRegionId(DisplayRegionId regionId) {
    for (size_t i = 0; i < regionRefreshStateCache.count; i++) {
        if (regionRefreshStateCache.states[i].regionId == regionId) {
            return i;
        }
    }

    return -1;
}
