#include "esp_log.h"
#include "epaper_port.h"
#include "epaper_bsp.h"

#include "../environment_types.h"
#include "../display_types.h"
#include "../screens/home_screen.h"
#include "../app_event.h"
#include "../app_store.h"
#include "../screen_manager.h"


// Define the grid dimensions
static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 5,
    .rows = 4
};

static DisplayRegionDescriptor displayRegions[] = {
    [DISPLAY_REGION_CLOCK] = {  .id = DISPLAY_REGION_CLOCK },
    [DISPLAY_REGION_TEMPERATURE] = { .id = DISPLAY_REGION_TEMPERATURE },
    [DISPLAY_REGION_HUMIDITY] = { .id = DISPLAY_REGION_HUMIDITY },
    [DISPLAY_REGION_ALERT] = { .id = DISPLAY_REGION_ALERT }
};

static int cellWidth;
static int cellHeight;

// Define the data cache area of the e-ink screen
uint8_t *ImageMonoBuffer;

// Log tag
static const char *TAG = "display";

static enum display_error initDisplay(void);
static void renderToDisplay(DisplayState *displayState);
static void partialRenderToDisplay(DisplayState *displayState);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *appState);
static ScreenRenderResult evaluateDisplay(const AppState *appState);
static bool isDisplayStateChanged(const DisplayState *left, const DisplayState *right);
static bool isClockStateChanged(const DisplayState *left, const DisplayState *right);
static bool isTemperatureStateChanged(const DisplayState *left, const DisplayState *right);
static bool isHumidityStateChanged(const DisplayState *left, const DisplayState *right);
static void initRenderGrid(void);
static void initRenderRegions(void);
static void renderRegionToDisplay(struct GridRegion region, struct PixelCoordinates2D textPosition, const char displayText[16], sFONT *font);
static struct PixelCoordinates2D pixelRegionCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopRight(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelRegion regionToPixelSpace(struct GridRegion gridRegion);
static void updateMonoImageBuffer(struct PixelRegion pixelRegion, uint8_t *ImagePartialMono, uint8_t *ImageMonoBuffer);
static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate stateCurrentTime);
static void setLastEnqueuedDisplayState(const DisplayState *displayState);
static void deinitDisplay(void);

static DisplayState lastPaintedDisplayState = {0};
static DisplayState lastEnqueuedDisplayState = {0};

const ScreenInterface *homeScreen_getScreenInterface(void) {
    static const ScreenInterface screenInterface = {
        .init = initDisplay,
        .fullRenderToDisplay = renderToDisplay,
        .partialRenderToDisplay = partialRenderToDisplay,
        .handleEvent = handleEvent,
        .setLastEnqueuedDisplayState = setLastEnqueuedDisplayState,
        .evaluateDisplay = evaluateDisplay,
        .deinit = deinitDisplay,
    };

    return &screenInterface;
}

static enum display_error initDisplay(void)
{
    if((ImageMonoBuffer = (UBYTE *)malloc(EPD_SIZE_MONO)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return DISPLAY_FAIL;
    }

    initRenderGrid();
    initRenderRegions();

    return DISPLAY_SUCCESS;
}

static void partialRenderToDisplay(DisplayState *displayState)
{
    ESP_LOGI(TAG,"4.e-Paper Partial Draw 0...");

    if (!isDisplayStateChanged(&lastPaintedDisplayState, displayState)) {
        ESP_LOGI(TAG,"Display state has not changed, skipping render.");
        return;
    }

    if (isClockStateChanged(&lastPaintedDisplayState, displayState)) {
        const struct GridRegion clockRegion = displayRegions[DISPLAY_REGION_CLOCK].gridRegion;
        const struct PixelRegion pixelRegion = displayRegions[DISPLAY_REGION_CLOCK].pixelRegion;
        const struct PixelSize2D textBoxSize = (struct PixelSize2D){ .width = strlen(displayState->clockText) * Font18.Width, .height = Font18.Height };
        struct PixelCoordinates2D clockTextPosition = { .x = pixelRegion.width - textBoxSize.width, .y = 0 };
        renderRegionToDisplay(clockRegion, clockTextPosition, displayState->clockText, &Font18);
    }

    if (isTemperatureStateChanged(&lastPaintedDisplayState, displayState)) {
        const struct GridRegion temperatureRegion = displayRegions[DISPLAY_REGION_TEMPERATURE].gridRegion;
        const struct PixelRegion pixelTemperatureRegion = displayRegions[DISPLAY_REGION_TEMPERATURE].pixelRegion;
        const struct PixelSize2D textBoxSize = (struct PixelSize2D){ .width = strlen(displayState->temperatureText) * Font48.Width, .height = Font48.Height };
        struct PixelCoordinates2D temperatureTextPosition = { .x = (pixelTemperatureRegion.width - textBoxSize.width) / 2, .y = (pixelTemperatureRegion.height - textBoxSize.height) / 2 };
        renderRegionToDisplay(temperatureRegion, temperatureTextPosition, displayState->temperatureText, &Font48);
    }

    if(isHumidityStateChanged(&lastPaintedDisplayState, displayState)) {
        const struct GridRegion humidityRegion = displayRegions[DISPLAY_REGION_HUMIDITY].gridRegion;
        const struct PixelRegion pixelHumidityRegion = displayRegions[DISPLAY_REGION_HUMIDITY].pixelRegion;
        const struct PixelSize2D humidityTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->humidityText) * Font16.Width, .height = Font16.Height };
        struct PixelCoordinates2D humidityTextPosition = { .x = (pixelHumidityRegion.width - humidityTextBoxSize.width) / 2, .y = 0 };
        renderRegionToDisplay(humidityRegion, humidityTextPosition, displayState->humidityText, &Font16);
    }
    
    lastPaintedDisplayState = *displayState;
}

static void renderToDisplay(DisplayState *displayState)
{
    ESP_LOGI(TAG,"3.e-Paper Draw 0...");

    if (!isDisplayStateChanged(&lastPaintedDisplayState, displayState)) {
        ESP_LOGI(TAG,"Display state has not changed, skipping render.");
        return;
    }

    const struct GridRegion clockRegion = displayRegions[DISPLAY_REGION_CLOCK].gridRegion;
    const struct GridRegion temperatureRegion = displayRegions[DISPLAY_REGION_TEMPERATURE].gridRegion;
    const struct GridRegion humidityRegion = displayRegions[DISPLAY_REGION_HUMIDITY].gridRegion;

    const struct PixelRegion clockPixelRegion = regionToPixelSpace(clockRegion);
    const struct PixelSize2D clockTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->clockText) * Font18.Width, .height = Font18.Height };
    const struct PixelCoordinates2D clockPixelCoordinates = pixelRegionTopRight(clockPixelRegion, clockTextBoxSize);

    const struct PixelRegion temperaturePixelRegion = regionToPixelSpace(temperatureRegion);
    const struct PixelSize2D temperatureTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->temperatureText) * Font48.Width, .height = Font48.Height };
    const struct PixelCoordinates2D temperaturePixelCoordinates = pixelRegionCenter(temperaturePixelRegion, temperatureTextBoxSize);

    const struct PixelRegion humidityPixelRegion = regionToPixelSpace(humidityRegion);
    const struct PixelSize2D humidityTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->humidityText) * Font16.Width, .height = Font16.Height };
    const struct PixelCoordinates2D humidityPixelCoordinates = pixelRegionTopCenter(humidityPixelRegion, humidityTextBoxSize);

    Paint_NewImage(ImageMonoBuffer, gridConfig.width, gridConfig.height, ROTATE_0, WHITE);
    Paint_SelectImage(ImageMonoBuffer);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(clockPixelCoordinates.x, clockPixelCoordinates.y, displayState->clockText, &Font18, WHITE, BLACK);
    Paint_DrawString_EN(temperaturePixelCoordinates.x, temperaturePixelCoordinates.y, displayState->temperatureText, &Font48, WHITE, BLACK);
    Paint_DrawString_EN(humidityPixelCoordinates.x, humidityPixelCoordinates.y, displayState->humidityText, &Font16, WHITE, BLACK);

    EPD_Display_Base(ImageMonoBuffer);

    lastPaintedDisplayState = *displayState;
}

static void setLastEnqueuedDisplayState(const DisplayState *displayState) {
    lastEnqueuedDisplayState = *displayState;
}

static ScreenActionResult handleEvent(const AppEvent *event, const AppState *appState) {
    ScreenActionResult result = {
        .screenIntent = {
            .intentType = SCREEN_INTENT_TYPE_NONE
        }
    };

    return result;
}

static ScreenRenderResult evaluateDisplay(const AppState *appState) {
    ScreenRenderResult result = { .isRenderRequired = false };

    DisplayState newDisplayState = readyDisplayState(
        appState->sharedState.environmentState.temperatureC, 
        appState->sharedState.environmentState.relativeHumidity, 
        appState->sharedState.environmentState.currentTime
    );

    if (isDisplayStateChanged(&lastEnqueuedDisplayState, &newDisplayState)) {
        result.isRenderRequired = true;
        result.displayState = newDisplayState;
    }

    return result;
}

static enum display_error initRegionMonoImage(struct GridRegion gridRegion, uint8_t **monoRegionImage) {
    const struct PixelRegion pixelRegion = regionToPixelSpace(gridRegion);
    const int regionImageSize = ((pixelRegion.width + 7) / 8) * pixelRegion.height; // Calculate the size in bytes for a monochrome image

    *monoRegionImage = (UBYTE *)malloc(regionImageSize);
    if(*monoRegionImage == NULL) {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return DISPLAY_FAIL;
    }

    return DISPLAY_SUCCESS;
}

static bool isClockStateChanged(const DisplayState *left, const DisplayState *right) {
    return strcmp(left->clockText, right->clockText) != 0;
}

static bool isTemperatureStateChanged(const DisplayState *left, const DisplayState *right) {
    return strcmp(left->temperatureText, right->temperatureText) != 0;
}

static bool isHumidityStateChanged(const DisplayState *left, const DisplayState *right) {
    return strcmp(left->humidityText, right->humidityText) != 0;
}

static bool isSensorStateChanged(const DisplayState *left, const DisplayState *right) {
    return isTemperatureStateChanged(left, right) || isHumidityStateChanged(left, right);
}

static bool isDisplayStateChanged(const DisplayState *left, const DisplayState *right) {
    return isTemperatureStateChanged(left, right) 
    || isHumidityStateChanged(left, right) 
    || isClockStateChanged(left, right)
    ;
}

static void renderRegionToDisplay(struct GridRegion displayRegion, struct PixelCoordinates2D textPosition, const char displayText[16], sFONT *font) {
        const struct PixelRegion pixelRegion = regionToPixelSpace(displayRegion);
        uint8_t *ImagePartialMono;

        initRegionMonoImage(displayRegion, &ImagePartialMono);

        Paint_NewImage(ImagePartialMono, pixelRegion.width, pixelRegion.height, ROTATE_0, WHITE);
        Paint_SelectImage(ImagePartialMono);
        Paint_Clear(WHITE);
        Paint_DrawString_EN(textPosition.x, textPosition.y, displayText, font, WHITE, BLACK);
        
        ESP_LOGI(TAG,
            "Partial rendering region x:%d y:%d width:%d height:%d; text: %s", 
            pixelRegion.x, pixelRegion.y, pixelRegion.width, pixelRegion.height, displayText
        );

        updateMonoImageBuffer(pixelRegion, ImagePartialMono, ImageMonoBuffer);


        /*

        When the driver supports partial update, use the code below to update the corresponding region of the screen. 
        Otherwise, use full screen partial update. The driver first needs to be able to update the region and snyc the
        buffer with the region image, then partial update can be used to update the region on screen.

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

        free(ImagePartialMono);
}

static void initRenderGrid(void)
{
    // Initialize the rendering grid or any necessary data structures here
    cellWidth = gridConfig.width / gridConfig.columns;
    cellHeight = gridConfig.height / gridConfig.rows;    
}

static void initRenderRegions(void)
{
    // Initialize the rendering regions based on the grid configuration
    // You can create an array of GridRegion structures to define specific areas for rendering

    displayRegions[DISPLAY_REGION_CLOCK].gridRegion = (struct GridRegion){ .x = 4, .y = 0, .width = 1, .height = 1 };
    displayRegions[DISPLAY_REGION_TEMPERATURE].gridRegion = (struct GridRegion){ .x = 1, .y = 1, .width = 3, .height = 2 };
    displayRegions[DISPLAY_REGION_HUMIDITY].gridRegion = (struct GridRegion){ .x = 1, .y = 3, .width = 3, .height = 1 };
    displayRegions[DISPLAY_REGION_ALERT].gridRegion = (struct GridRegion){.x = 0, .y = 1, .width = 1, .height = 2};

    const int numberOfRegions = sizeof(displayRegions) / sizeof(displayRegions[0]);
    for (size_t i = 0; i < numberOfRegions; i++) {
        ESP_LOGI(TAG, "Initializing region %d", i);
        displayRegions[i].pixelRegion = regionToPixelSpace(displayRegions[i].gridRegion);
    }
}

static struct PixelCoordinates2D pixelRegionCenter(struct PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y + (pixelRegion.height - pixelItemSize.height) / 2;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static struct PixelCoordinates2D pixelRegionTopCenter(struct PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static struct PixelCoordinates2D pixelRegionTopRight(struct PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + pixelRegion.width - pixelItemSize.width;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static struct PixelRegion regionToPixelSpace(struct GridRegion gridRegion) {
    const int pixelX = gridRegion.x * cellWidth;
    const int pixelY = gridRegion.y * cellHeight;
    const int pixelWidth = gridRegion.width * cellWidth;
    const int pixelHeight = gridRegion.height * cellHeight;

    return (struct PixelRegion){ .x = pixelX, .y = pixelY, .width = pixelWidth, .height = pixelHeight };
}

static void updateMonoImageBuffer(struct PixelRegion pixelRegion, uint8_t *ImagePartialMono, uint8_t *ImageMonoBuffer) {
    const int fullStride = (gridConfig.width + 7) / 8;
    const int regionStride = (pixelRegion.width + 7) / 8;
    const int xByteOffset = pixelRegion.x / 8;

    for (int row = 0; row < pixelRegion.height; row++) {
        memcpy(
            ImageMonoBuffer + ((pixelRegion.y + row) * fullStride) + xByteOffset,
            ImagePartialMono + (row * regionStride),
            regionStride
        );
    }
}

static DisplayState readyDisplayState(float stateTemperatureC, float stateRelativeHumidity, TimeDate stateCurrentTime) {
    DisplayState displayState = {0};
    snprintf(displayState.temperatureText, sizeof(displayState.temperatureText), "%.1fC", stateTemperatureC);
    snprintf(displayState.humidityText, sizeof(displayState.humidityText), "%.1f%%", stateRelativeHumidity);

    if (stateCurrentTime.hours > 23 || stateCurrentTime.minutes > 59) {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "--:--");
    } else {
        snprintf(displayState.clockText, sizeof(displayState.clockText), "%02u:%02u", (unsigned)stateCurrentTime.hours, (unsigned)stateCurrentTime.minutes);
    }

    displayState.showEnvironmentWarning = false;
    displayState.showBluetooth = false;
    displayState.showWifi = false;
    displayState.showBattery = false;

    return displayState;
}

static void deinitDisplay(void) {
    if (ImageMonoBuffer != NULL) {
        free(ImageMonoBuffer);
        ImageMonoBuffer = NULL;
    }

    lastPaintedDisplayState = (DisplayState){0};
    lastEnqueuedDisplayState = (DisplayState){0};
}