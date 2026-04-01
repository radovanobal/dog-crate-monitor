#include "esp_log.h"
#include "epaper_port.h"

#include "../environment_types.h"
#include "../display_types.h"
#include "../screens/home_screen.h"
#include "../app_event.h"
#include "../app_store.h"
#include "../screen_manager.h"


typedef enum {
    REGION_ALIGNMENT_CENTER = 0,
    REGION_ALIGNMENT_TOP_CENTER = 1,
    REGION_ALIGNMENT_TOP_RIGHT = 2
} RegionAlignment;

static int cellWidth;
static int cellHeight;

static TimeDate lastTime = {0};
static float lastTemperatureC = 0.0f;
static float lastHumidity = 0.0f;


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

// Log tag
static const char *TAG = "home_screen";

static void initDisplay(void);
static void initRenderGrid(void);
static void initRenderRegions(void);
static void deinitDisplay(void);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *appState);
static ScreenRenderResult evaluateDisplay(const AppState *appState);
static PixelRegion regionToPixelSpace(struct GridRegion gridRegion);
static PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, PixelRegion pixelRegion, char text[16], sFONT *font);
static struct PixelCoordinates2D pixelRegionCenter(PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopCenter(PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D pixelRegionTopRight(PixelRegion pixelRegion, struct PixelSize2D itemSize);
static struct PixelCoordinates2D buildPixelCoordinates(DisplayRegionId regionId, const char displayText[16], const sFONT *font, RegionAlignment alignment);
static char* buildClockText(TimeDate currentTime, char *buffer, size_t bufferSize);
static char* buildTemperatureText(float temperatureC, char *buffer, size_t bufferSize);
static char* buildHumidityText(float humidity, char *buffer, size_t bufferSize);
static bool isTimeDateEqual(TimeDate t1, TimeDate t2);

const ScreenInterface *homeScreen_getScreenInterface(void) {
    static const ScreenInterface screenInterface = {
        .init = initDisplay,
        .handleEvent = handleEvent,
        .evaluateDisplay = evaluateDisplay,
        .deinit = deinitDisplay,
    };

    return &screenInterface;
}

static void initDisplay(void)
{
    ESP_LOGI(TAG, "Initializing display and render regions");
    initRenderGrid();
    initRenderRegions();
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
    ScreenRenderResult result = {0};
    DisplayRenderPlan displayRenderPlan = {0};

    int renderItemIndex = 0;

    if(!isTimeDateEqual(lastTime, appState->sharedState.environmentState.currentTime)) {
        char clockText[16];
        buildClockText(appState->sharedState.environmentState.currentTime, clockText, sizeof(clockText));
        struct PixelCoordinates2D clockTextPosition = buildPixelCoordinates(DISPLAY_REGION_CLOCK, clockText, &Font18, REGION_ALIGNMENT_TOP_RIGHT);
        displayRenderPlan.items[renderItemIndex++] = createTextRenderItem(clockTextPosition, displayRegions[DISPLAY_REGION_CLOCK].pixelRegion, clockText, &Font18);
    }

    if(lastTemperatureC != appState->sharedState.environmentState.temperatureC) {
        char temperatureText[16];
        buildTemperatureText(appState->sharedState.environmentState.temperatureC, temperatureText, sizeof(temperatureText));
        struct PixelCoordinates2D temperatureTextPosition = buildPixelCoordinates(DISPLAY_REGION_TEMPERATURE, temperatureText, &Font48, REGION_ALIGNMENT_CENTER);
        displayRenderPlan.items[renderItemIndex++] = createTextRenderItem(temperatureTextPosition, displayRegions[DISPLAY_REGION_TEMPERATURE].pixelRegion, temperatureText, &Font48);
    }

    if(lastHumidity != appState->sharedState.environmentState.relativeHumidity) {
        char humidityText[16];
        buildHumidityText(appState->sharedState.environmentState.relativeHumidity, humidityText, sizeof(humidityText));
        struct PixelCoordinates2D humidityTextPosition = buildPixelCoordinates(DISPLAY_REGION_HUMIDITY, humidityText, &Font16, REGION_ALIGNMENT_TOP_CENTER);
        displayRenderPlan.items[renderItemIndex++] = createTextRenderItem(humidityTextPosition, displayRegions[DISPLAY_REGION_HUMIDITY].pixelRegion, humidityText, &Font16);    
    }

    displayRenderPlan.count = renderItemIndex;
    result.displayRenderPlan = displayRenderPlan;


    lastTime = appState->sharedState.environmentState.currentTime;
    lastTemperatureC = appState->sharedState.environmentState.temperatureC;
    lastHumidity = appState->sharedState.environmentState.relativeHumidity;

    return result;
}

static bool isTimeDateEqual(TimeDate t1, TimeDate t2) {
    return t1.years == t2.years &&
           t1.months == t2.months &&
           t1.days == t2.days &&
           t1.hours == t2.hours &&
           t1.minutes == t2.minutes &&
           t1.seconds == t2.seconds &&
           t1.week == t2.week;
}

static char* buildClockText(TimeDate currentTime, char *buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%02d:%02d", currentTime.hours, currentTime.minutes);
    return buffer;
}

static char* buildTemperatureText(float temperatureC, char *buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.1fC", temperatureC);
    return buffer;
}

static char* buildHumidityText(float humidity, char *buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.1f%%", humidity);
    return buffer;
}

static struct PixelCoordinates2D buildPixelCoordinates(DisplayRegionId regionId, const char displayText[16], const sFONT *font, RegionAlignment alignment) {
    const PixelRegion pixelRegion = displayRegions[regionId].pixelRegion;
    const struct PixelSize2D textBoxSize = (struct PixelSize2D){ .width = strlen(displayText) * font->Width, .height = font->Height };
    struct PixelCoordinates2D textPosition;

    switch (alignment) {
        case REGION_ALIGNMENT_CENTER:
            textPosition = pixelRegionCenter(pixelRegion, textBoxSize);
            break;
        case REGION_ALIGNMENT_TOP_CENTER:
            textPosition = pixelRegionTopCenter(pixelRegion, textBoxSize);
            break;
        case REGION_ALIGNMENT_TOP_RIGHT:
            textPosition = pixelRegionTopRight(pixelRegion, textBoxSize);
            break;
        default:
            textPosition = pixelRegionCenter(pixelRegion, textBoxSize);
            break;
    }

    return textPosition;
}

static PixelRenderItem createTextRenderItem(struct PixelCoordinates2D position, PixelRegion pixelRegion, char text[16], sFONT *font) {
    PixelRenderItem renderItem = (PixelRenderItem){
        .type = RENDER_ITEM_TYPE_TEXT,
        .pixelRegion = pixelRegion,
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

static void initRenderGrid(void)
{
    cellWidth = gridConfig.width / gridConfig.columns;
    cellHeight = gridConfig.height / gridConfig.rows;    
}

static void initRenderRegions(void)
{
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

static struct PixelCoordinates2D pixelRegionCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y + (pixelRegion.height - pixelItemSize.height) / 2;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static struct PixelCoordinates2D pixelRegionTopCenter(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + (pixelRegion.width - pixelItemSize.width) / 2;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static struct PixelCoordinates2D pixelRegionTopRight(PixelRegion pixelRegion, struct PixelSize2D pixelItemSize) {
    const UWORD x = pixelRegion.x + pixelRegion.width - pixelItemSize.width;
    const UWORD y = pixelRegion.y;

    return (struct PixelCoordinates2D){ .x = x, .y = y };
}

static PixelRegion regionToPixelSpace(struct GridRegion gridRegion) {
    const int pixelX = gridRegion.x * cellWidth;
    const int pixelY = gridRegion.y * cellHeight;
    const int pixelWidth = gridRegion.width * cellWidth;
    const int pixelHeight = gridRegion.height * cellHeight;

    return (PixelRegion){ .x = pixelX, .y = pixelY, .width = pixelWidth, .height = pixelHeight };
}

static void deinitDisplay(void) {
    lastHumidity = 0.0f;
    lastTemperatureC = 0.0f;
    lastTime = (TimeDate){0};

    ESP_LOGI(TAG, "Display deinitialized and state reset");
}