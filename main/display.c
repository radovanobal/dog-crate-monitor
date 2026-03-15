#include "esp_log.h"
#include "epaper_port.h"
#include "epaper_bsp.h"

#include "./environment_types.h"
#include "./display_types.h"
#include "./display.h"


// Define the grid dimensions
static const struct GridConfig gridConfig = {
    .width = 800,
    .height = 480,
    .columns = 5,
    .rows = 4
};

static int cellWidth;
static int cellHeight;

static struct GridRegion temperatureRegion;
static struct GridRegion humidityRegion;
static struct GridRegion clockRegion;

// Define the data cache area of the e-ink screen
uint8_t *Image_Mono;
// Log tag
static const char *TAG = "main";

static void initRenderGrid(void);
static void initRenderRegions(void);
static struct PixelCoordinates2D pixelRegionCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopRight(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelRegion regionToPixelSpace(struct GridRegion gridRegion);

static DisplayState currentDisplayState = {0};

enum display_error initDisplay(void)
{
    ESP_LOGI(TAG,"1.e-Paper Init and Clear...");
    epaper_port_init();
    EPD_Init();
    EPD_Clear();
    vTaskDelay(pdMS_TO_TICKS(2000));

    if((Image_Mono = (UBYTE *)malloc(EPD_SIZE_MONO)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
        return DISPLAY_FAIL;
    }

    initRenderGrid();
    initRenderRegions();

    return DISPLAY_SUCCESS;
}

static bool isDisplayStateChanged(const DisplayState *left, const DisplayState *right) {
    return strcmp(left->temperatureText, right->temperatureText) != 0 
        || strcmp(left->humidityText, right->humidityText) != 0 
        || strcmp(left->clockText, right->clockText) != 0
        ;
}

void renderToDisplay(DisplayState *displayState)
{
    ESP_LOGI(TAG,"3.e-Paper Draw 0...");

    if (!isDisplayStateChanged(&currentDisplayState, displayState)) {
        ESP_LOGI(TAG,"Display state has not changed, skipping render.");
        return;
    }

    const struct PixelRegion clockPixelRegion = regionToPixelSpace(clockRegion);
    const struct PixelSize2D clockTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->clockText) * Font18.Width, .height = Font18.Height };
    const struct PixelCoordinates2D clockPixelCoordinates = pixelRegionTopRight(clockPixelRegion, clockTextBoxSize);

    const struct PixelRegion temperaturePixelRegion = regionToPixelSpace(temperatureRegion);
    const struct PixelSize2D temperatureTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->temperatureText) * Font48.Width, .height = Font48.Height };
    const struct PixelCoordinates2D temperaturePixelCoordinates = pixelRegionCenter(temperaturePixelRegion, temperatureTextBoxSize);

    const struct PixelRegion humidityPixelRegion = regionToPixelSpace(humidityRegion);
    const struct PixelSize2D humidityTextBoxSize = (struct PixelSize2D){ .width = strlen(displayState->humidityText) * Font16.Width, .height = Font16.Height };
    const struct PixelCoordinates2D humidityPixelCoordinates = pixelRegionTopCenter(humidityPixelRegion, humidityTextBoxSize);

    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(Image_Mono);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(clockPixelCoordinates.x, clockPixelCoordinates.y, displayState->clockText, &Font18, WHITE, BLACK);
    Paint_DrawString_EN(temperaturePixelCoordinates.x, temperaturePixelCoordinates.y, displayState->temperatureText, &Font48, WHITE, BLACK);
    Paint_DrawString_EN(humidityPixelCoordinates.x, humidityPixelCoordinates.y, displayState->humidityText, &Font16, WHITE, BLACK);

    EPD_display(Image_Mono);

    currentDisplayState = *displayState;
}

static void initRenderGrid(void)
{
    // Initialize the rendering grid or any necessary data structures here
    // For this example, we are not using a specific grid, but you can set up any required structures
    cellWidth = gridConfig.width / gridConfig.columns;
    cellHeight = gridConfig.height / gridConfig.rows;    
}

static void initRenderRegions(void)
{
    // Initialize the rendering regions based on the grid configuration
    // For this example, we are not defining specific regions, but you can set up any required structures
    // You can create an array of GridRegion structures to define specific areas for rendering
    clockRegion = (struct GridRegion){ .x = 4, .y = 0, .width = 1, .height = 1 };
    temperatureRegion = (struct GridRegion){ .x = 1, .y = 1, .width = 3, .height = 2 };
    humidityRegion = (struct GridRegion){ .x = 1, .y = 3, .width = 3, .height = 1 };
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