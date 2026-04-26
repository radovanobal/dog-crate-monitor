#include "GUI_Paint.h"
#include "epaper_port.h"

#include "display_pipeline_gray4.h"


static const char *TAG = "display_pipeline_gray4";

static uint8_t *ImageGrayscaleBuffer;

static display_init_error init(void);
static void deinit(void);
static void prepareBuffer(const DisplayRenderPlan *displayRenderPlan, const DisplayPaintType displayPaintType);
static void flushBufferToDisplay(const DisplayPaintType displayPaintType);
static uint16_t getColor(DisplayColor color);

DisplayPipelineInterface* displayPipelineGray4_getPipelineInterface() {
    static const DisplayPipelineInterface pipelineInterface = {
        .init = init,
        .deinit = deinit,
        .prepareBuffer = prepareBuffer,
        .flushBufferToDisplay = flushBufferToDisplay,
        .getColor = getColor,
        .pipelineType = DISPLAY_PIPELINE_TYPE_GRAYSCALE
    };

    return (DisplayPipelineInterface*)&pipelineInterface;
};

static display_init_error init(void) {
    EPD_Init_4GRAY();

    if((ImageGrayscaleBuffer = (UBYTE *)malloc(EPD_SIZE_4GRAY)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for grayscale memory...");
        return DISPLAY_FAIL;
    }

    return DISPLAY_SUCCESS;
}

static void deinit(void) {
    if (ImageGrayscaleBuffer) {
        free(ImageGrayscaleBuffer);
        ImageGrayscaleBuffer = NULL;
    }
    EPD_Sleep();
}

static void prepareBuffer(const DisplayRenderPlan *displayRenderPlan, const DisplayPaintType displayPaintType) {
    Paint_NewImage(ImageGrayscaleBuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, GRAY1);
    Paint_SetScale(4);
    Paint_SelectImage(ImageGrayscaleBuffer);
    Paint_Clear(GRAY1);
}

static void flushBufferToDisplay(const DisplayPaintType displayPaintType) {
    EPD_Display_4Gray(ImageGrayscaleBuffer);
}

static uint16_t getColor(DisplayColor color) {
    switch (color) {
        case DISPLAY_COLOR_BLACK:
            return GRAY4;
        case DISPLAY_COLOR_GRAY1:
            return GRAY3;
        case DISPLAY_COLOR_GRAY2:
            return GRAY2;
        case DISPLAY_COLOR_WHITE:
            return GRAY1;
        default:
            ESP_LOGW(TAG, "Unknown display color: %d. Defaulting to WHITE.", color);
            return GRAY1;
    }
}
