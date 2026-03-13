#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epaper_port.h"
#include "epaper_bsp.h"
#include "esp_log.h"
#include "i2c_bsp.h"
#include "shtc3_bsp.h"

struct GridConfig {
    int width;
    int height;
    int columns;
    int rows;
};

struct GridRegion {
    int x;
    int y;
    int width;
    int height;
};

struct PixelRegion {
    int x;
    int y;
    int width;
    int height;
};

struct PixelSize2D {
    int width;
    int height;
};

struct PixelCoordinates2D {
    UWORD x;
    UWORD y;
};

static void initDisplay(void);
static void initI2C(void);
static void initRenderGrid(void);
static void initRenderRegions(void);
static void readTemperatureAndHumidity(void);
static void renderToDisplay(void);
static struct PixelCoordinates2D pixelRegionCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopCenter(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopRigth(struct PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelRegion regionToPixelSpace(struct GridRegion gridRegion);

// Define the grid dimensions
static const struct GridConfig gridConfig = {
    .width = 800,
    .height = 480,
    .columns = 5,
    .rows = 4
};


// Define the data cache area of the e-ink screen
uint8_t *Image_Mono;
// Log tag
static const char *TAG = "main";

static float stateTemparatureC;
static float stateRelativeHumidity;
static int cellWidth;
static int cellHeight;

static struct GridRegion temperatureRegion;
static struct GridRegion humidityRegion;
static struct GridRegion clockRegion;

void app_main(void)
{
    initI2C();
    initDisplay();
    initRenderGrid();
    initRenderRegions();

    while(true) {
        readTemperatureAndHumidity();
        renderToDisplay();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute before the next reading
    }

}

static void initDisplay(void)
{
    ESP_LOGI(TAG,"1.e-Paper Init and Clear...");
    epaper_port_init();
    EPD_Init();
    EPD_Clear();
    vTaskDelay(pdMS_TO_TICKS(2000));

    if((Image_Mono = (UBYTE *)malloc(EPD_SIZE_MONO)) == NULL)
    {
        ESP_LOGE(TAG,"Failed to apply for black memory...");
    }

}

static void initI2C(void)
{
    i2c_master_init();
    i2c_shtc3_init();
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

static void readTemperatureAndHumidity(void)
{
    ESP_LOGI(TAG,"2.Temperature sensor read...");
    SHTC3_GetEnvTemperatureHumidity(&stateTemparatureC, &stateRelativeHumidity);
}



static struct PixelRegion regionToPixelSpace(struct GridRegion gridRegion) {
    const int pixelX = gridRegion.x * cellWidth;
    const int pixelY = gridRegion.y * cellHeight;
    const int pixelWidth = gridRegion.width * cellWidth;
    const int pixelHeight = gridRegion.height * cellHeight;

    return (struct PixelRegion){ .x = pixelX, .y = pixelY, .width = pixelWidth, .height = pixelHeight };
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

static void renderToDisplay(void)
{
    ESP_LOGI(TAG,"3.e-Paper Draw 0...");

    char temperatureText[16] = "";
    char humidityText[16] = "";

    snprintf(temperatureText, sizeof(temperatureText), "%.1f°C", stateTemparatureC);
    snprintf(humidityText, sizeof(humidityText), "%.1f%%", stateRelativeHumidity);

    const struct PixelRegion temperaturePixelRegion = regionToPixelSpace(temperatureRegion);
    const struct PixelSize2D temperatureTextBoxSize = (struct PixelSize2D){ .width = strlen(temperatureText) * Font48.Width, .height = Font48.Height };
    const struct PixelCoordinates2D temperaturePixelCoordinates = pixelRegionCenter(temperaturePixelRegion, temperatureTextBoxSize);

    const struct PixelRegion humidityPixelRegion = regionToPixelSpace(humidityRegion);
    const struct PixelSize2D humidityTextBoxSize = (struct PixelSize2D){ .width = strlen(humidityText) * Font16.Width, .height = Font16.Height };
    const struct PixelCoordinates2D humidityPixelCoordinates = pixelRegionTopCenter(humidityPixelRegion, humidityTextBoxSize);

    Paint_NewImage(Image_Mono, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(Image_Mono);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(temperaturePixelCoordinates.x, temperaturePixelCoordinates.y, temperatureText, &Font48, WHITE, BLACK);
    Paint_DrawString_EN(humidityPixelCoordinates.x, humidityPixelCoordinates.y, humidityText, &Font16, WHITE, BLACK);

    EPD_display(Image_Mono);
}