#include "esp_log.h"
#include "epaper_port.h"

#include "../environment_types.h"
#include "../display_types.h"
#include "../app_event.h"
#include "../app_store.h"
#include "../screen_manager.h"
#include "../screen_layout.h"
#include "../screen_render.h"
#include "../utils/macros.h"
#include "screen_types.h"
#include "../screens/home_screen.h"


typedef struct {
    struct {
        TimeDate currentTime;
        float temperatureC;
        float relativeHumidity;
    } data;
    struct {
        char clockText[16];
        char temperatureText[16];
        char humidityText[16];
    } derived;
} HomeScreenState;

static ScreenLayout screenLayout;
static HomeScreenState homeScreenState = {0};
static HomeScreenState nextScreenState = {0};

static const DisplayRegionId DISPLAY_REGION_CLOCK = 0;
static const DisplayRegionId DISPLAY_REGION_TEMPERATURE = 1;
static const DisplayRegionId DISPLAY_REGION_HUMIDITY = 2;
static const DisplayRegionId DISPLAY_REGION_ALERT = 3;

enum {
    HOME_REGION_SLOT_CLOCK = 0,
    HOME_REGION_SLOT_TEMPERATURE = 1,
    HOME_REGION_SLOT_HUMIDITY = 2,
    HOME_REGION_SLOT_ALERT = 3,
};

// Define the grid dimensions
static const struct GridConfig gridConfig = {
    .width = EPD_WIDTH,
    .height = EPD_HEIGHT,
    .columns = 5,
    .rows = 4
};

static DisplayRegionDescriptor displayRegions[] = {
    [HOME_REGION_SLOT_CLOCK] = {  .id = DISPLAY_REGION_CLOCK },
    [HOME_REGION_SLOT_TEMPERATURE] = { .id = DISPLAY_REGION_TEMPERATURE },
    [HOME_REGION_SLOT_HUMIDITY] = { .id = DISPLAY_REGION_HUMIDITY },
    [HOME_REGION_SLOT_ALERT] = { .id = DISPLAY_REGION_ALERT }
};

static DirtyRegionEntry dirtyDisplayRegions[] = {
    [HOME_REGION_SLOT_CLOCK] = { .regionId = DISPLAY_REGION_CLOCK, .isDirty = false },
    [HOME_REGION_SLOT_TEMPERATURE] = { .regionId = DISPLAY_REGION_TEMPERATURE, .isDirty = false },
    [HOME_REGION_SLOT_HUMIDITY] = { .regionId = DISPLAY_REGION_HUMIDITY, .isDirty = false },
    [HOME_REGION_SLOT_ALERT] = { .regionId = DISPLAY_REGION_ALERT, .isDirty = false }
};

// Log tag
static const char *TAG = "home_screen";

static void initDisplay(const AppState *state);
static void initRenderRegions(void);
static void deinitDisplay(void);
static void derivedStateFromAppState(const AppState *appState);
static void determineDirtyRegions(void);
static DisplayRenderPlan buildDisplayRenderPlan(const AppState *appState);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *appState);
static ScreenRenderResult evaluateDisplay(const AppState *appState);
static bool isTimeDateEqual(TimeDate t1, TimeDate t2);
static PixelRenderItem createClockRenderItem(const AppState *appState);
static PixelRenderItem createTemperatureRenderItem(const AppState *appState);
static PixelRenderItem createHumidityRenderItem(const AppState *appState);

const ScreenInterface *homeScreen_getScreenInterface(void) {
    static const ScreenInterface screenInterface = {
        .purpose = SCREEN_PURPOSE_DATA_DISPLAY,
        .init = initDisplay,
        .handleEvent = handleEvent,
        .evaluateDisplay = evaluateDisplay,
        .deinit = deinitDisplay,
    };

    return &screenInterface;
}

static void initDisplay(const AppState *state)
{
    ESP_LOGI(TAG, "Initializing display and render regions");
    screenLayout = initRenderGrid(gridConfig);
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

    nextScreenState.data.currentTime = appState->sharedState.environmentState.currentTime;
    nextScreenState.data.temperatureC = appState->sharedState.environmentState.temperatureC;
    nextScreenState.data.relativeHumidity = appState->sharedState.environmentState.relativeHumidity;

    derivedStateFromAppState(appState);
    determineDirtyRegions();

    DisplayRenderPlan displayRenderPlan = buildDisplayRenderPlan(appState);

    result.displayRenderPlan = displayRenderPlan;

    homeScreenState = nextScreenState;
    return result;
}

static void derivedStateFromAppState(const AppState *appState) {
    snprintf(
        nextScreenState.derived.clockText, 
        sizeof(nextScreenState.derived.clockText), 
        "%02d:%02d",
        appState->sharedState.environmentState.currentTime.hours,
        appState->sharedState.environmentState.currentTime.minutes
    );

    snprintf(
        nextScreenState.derived.temperatureText, 
        sizeof(nextScreenState.derived.temperatureText), 
        "%.1fC", 
        appState->sharedState.environmentState.temperatureC
    );

    snprintf(
        nextScreenState.derived.humidityText, 
        sizeof(nextScreenState.derived.humidityText), 
        "%.1f%%", 
        appState->sharedState.environmentState.relativeHumidity
    );
}

static void determineDirtyRegions(void) {   
    for (size_t i = 0; i < ARRAY_SIZE(dirtyDisplayRegions); i++) {
        dirtyDisplayRegions[i].isDirty = false;
    }

    if(!isTimeDateEqual(homeScreenState.data.currentTime, nextScreenState.data.currentTime)) {
        dirtyDisplayRegions[HOME_REGION_SLOT_CLOCK].isDirty = strcmp(nextScreenState.derived.clockText, homeScreenState.derived.clockText) != 0;
    }

    if(homeScreenState.data.temperatureC != nextScreenState.data.temperatureC) {
        dirtyDisplayRegions[HOME_REGION_SLOT_TEMPERATURE].isDirty = strcmp(nextScreenState.derived.temperatureText, homeScreenState.derived.temperatureText) != 0;
    }

    if(homeScreenState.data.relativeHumidity != nextScreenState.data.relativeHumidity) {
        dirtyDisplayRegions[HOME_REGION_SLOT_HUMIDITY].isDirty = strcmp(nextScreenState.derived.humidityText, homeScreenState.derived.humidityText) != 0;
    }
}

static DisplayRenderPlan buildDisplayRenderPlan(const AppState *appState) {
    DisplayRenderPlan displayRenderPlan = {0};
    int sceneItemIndex = 0;

    if(dirtyDisplayRegions[HOME_REGION_SLOT_CLOCK].isDirty) {

        RenderRegionScene clockScene = {
            .regionId = DISPLAY_REGION_CLOCK,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_CLOCK].pixelRegion,
            .renderItems = {
                [0] = createClockRenderItem(appState)
            },
            .count = 1
        };

        displayRenderPlan.regions[sceneItemIndex++] = clockScene;
    }

    if(dirtyDisplayRegions[HOME_REGION_SLOT_TEMPERATURE].isDirty) {
        RenderRegionScene temperatureScene = {
            .regionId = DISPLAY_REGION_TEMPERATURE,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_TEMPERATURE].pixelRegion,
            .renderItems = {
                [0] = createTemperatureRenderItem(appState)
            },
            .count = 1
        };
        displayRenderPlan.regions[sceneItemIndex++] = temperatureScene;
    }

    if(dirtyDisplayRegions[HOME_REGION_SLOT_HUMIDITY].isDirty) {
        RenderRegionScene humidityScene = {
            .regionId = DISPLAY_REGION_HUMIDITY,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_HUMIDITY].pixelRegion,
            .renderItems = {
                [0] = createHumidityRenderItem(appState)
            },
            .count = 1
        };
        displayRenderPlan.regions[sceneItemIndex++] = humidityScene;
    }

    displayRenderPlan.count = sceneItemIndex;
    return displayRenderPlan;
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

static PixelRenderItem createClockRenderItem(const AppState *appState) {        
    ESP_LOGI(TAG, "Clock text changed from '%s' to '%s'", homeScreenState.derived.clockText, nextScreenState.derived.clockText);

    struct PixelCoordinates2D clockTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_CLOCK], nextScreenState.derived.clockText, &Font18, REGION_ALIGNMENT_TOP_RIGHT);
    PixelRenderItem renderItem = createTextRenderItem(clockTextPosition, nextScreenState.derived.clockText, &Font18);

    return renderItem;
}

static PixelRenderItem createTemperatureRenderItem(const AppState *appState) {
    ESP_LOGI(TAG, "Temperature text changed from '%s' to '%s'", homeScreenState.derived.temperatureText, nextScreenState.derived.temperatureText);

    struct PixelCoordinates2D temperatureTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_TEMPERATURE], nextScreenState.derived.temperatureText, &Font48, REGION_ALIGNMENT_CENTER);
    PixelRenderItem renderItem = createTextRenderItem(temperatureTextPosition, nextScreenState.derived.temperatureText, &Font48);

    return renderItem;
}

static PixelRenderItem createHumidityRenderItem(const AppState *appState) {
    ESP_LOGI(TAG, "Humidity text changed from '%s' to '%s'", homeScreenState.derived.humidityText, nextScreenState.derived.humidityText);

    struct PixelCoordinates2D humidityTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_HUMIDITY], nextScreenState.derived.humidityText, &Font16, REGION_ALIGNMENT_TOP_CENTER);
    PixelRenderItem renderItem = createTextRenderItem(humidityTextPosition, nextScreenState.derived.humidityText, &Font16);

    return renderItem;
}

static void initRenderRegions(void)
{
    displayRegions[HOME_REGION_SLOT_CLOCK].gridRegion = (struct GridRegion){ .x = 4, .y = 0, .width = 1, .height = 1 };
    displayRegions[HOME_REGION_SLOT_TEMPERATURE].gridRegion = (struct GridRegion){ .x = 1, .y = 1, .width = 3, .height = 2 };
    displayRegions[HOME_REGION_SLOT_HUMIDITY].gridRegion = (struct GridRegion){ .x = 1, .y = 3, .width = 3, .height = 1 };
    displayRegions[HOME_REGION_SLOT_ALERT].gridRegion = (struct GridRegion){.x = 0, .y = 1, .width = 1, .height = 2};

    const int regionCount = ARRAY_SIZE(displayRegions);
    calculateDisplayRegionsPixelSpace(displayRegions, regionCount, screenLayout);
}

static void deinitDisplay(void) {
    homeScreenState = (HomeScreenState){0};
    nextScreenState = (HomeScreenState){0};

    ESP_LOGI(TAG, "Display deinitialized and state reset");
}