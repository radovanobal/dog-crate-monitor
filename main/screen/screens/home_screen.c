#include <sys/stat.h>

#include "esp_log.h"
#include "epaper_port.h"

#include "app/app_event.h"
#include "app/app_store.h"
#include "display/display_types.h"
#include "screen/screen_manager.h"
#include "screen/screen_layout.h"
#include "screen/screen_render.h"
#include "screen/screen_types.h"
#include "screen/screens/home_screen.h"
#include "generated_icons.h"
#include "utils/macros.h"
#include "environment_types.h"


typedef struct {
    struct {
        TimeDate currentTime;
        float temperatureC;
        float relativeHumidity;
        int batteryLevel;
    } data;
    struct {
        char clockText[16];
        char temperatureText[16];
        char humidityText[16];
        char batteryLevelText[16];
    } derived;
} HomeScreenState;

static ScreenLayout screenLayout;
static HomeScreenState homeScreenState = {0};
static HomeScreenState nextScreenState = {0};

static const DisplayRegionId DISPLAY_REGION_CLOCK = 0;
static const DisplayRegionId DISPLAY_REGION_TEMPERATURE = 1;
static const DisplayRegionId DISPLAY_REGION_HUMIDITY = 2;
static const DisplayRegionId DISPLAY_REGION_ALERT = 3;
static const DisplayRegionId DISPLAY_REGION_BATTERY = 4;

enum {
    HOME_REGION_SLOT_CLOCK = 0,
    HOME_REGION_SLOT_TEMPERATURE = 1,
    HOME_REGION_SLOT_HUMIDITY = 2,
    HOME_REGION_SLOT_ALERT = 3,
    HOME_REGION_SLOT_BATTERY = 4
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
    [HOME_REGION_SLOT_ALERT] = { .id = DISPLAY_REGION_ALERT },
    [HOME_REGION_SLOT_BATTERY] = { .id = DISPLAY_REGION_BATTERY }
};

static DirtyRegionEntry dirtyDisplayRegions[] = {
    [HOME_REGION_SLOT_CLOCK] = { .regionId = DISPLAY_REGION_CLOCK, .isDirty = false },
    [HOME_REGION_SLOT_TEMPERATURE] = { .regionId = DISPLAY_REGION_TEMPERATURE, .isDirty = false },
    [HOME_REGION_SLOT_HUMIDITY] = { .regionId = DISPLAY_REGION_HUMIDITY, .isDirty = false },
    [HOME_REGION_SLOT_ALERT] = { .regionId = DISPLAY_REGION_ALERT, .isDirty = false },
    [HOME_REGION_SLOT_BATTERY] = { .regionId = DISPLAY_REGION_BATTERY, .isDirty = false }
};

// Log tag
static const char *TAG = "home_screen";

static void initDisplay(const AppState *state);
static void initRenderRegions(void);
static void deinitDisplay(void);
static void derivedStateFromAppState(const AppState *appState);
static void determineDirtyRegions(void);
static void buildDisplayRenderPlan(const AppState *appState, DisplayRenderPlan *displayRenderPlan);
static ScreenActionResult handleEvent(const AppEvent *event, const AppState *appState);
static DisplayPrepareRequest evaluateDisplay(const AppState *appState, DisplayRenderPlan *renderPlan, DisplayPipelineType *pipelineType);
static bool isTimeDateEqual(TimeDate t1, TimeDate t2);
static PixelRenderItem createClockRenderItem(const AppState *appState);
static PixelRenderItem createTemperatureRenderItem(const AppState *appState);
static PixelRenderItem createHumidityRenderItem(const AppState *appState);
static PixelRenderItem createBatteryLevelRenderItem(const AppState *appState);
static PixelRenderItem createTemperatureIconRenderItem(const AppState *appState);
static PixelRenderItem createHumidityIconRenderItem(const AppState *appState);
static PixelRenderItem createBatteryIconRenderItem(const AppState *appState);
static IconId selectTemperatureIcon(float temperatureC);
static IconId selectBatteryIcon(int batteryLevel);

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

static DisplayPrepareRequest evaluateDisplay(const AppState *appState, DisplayRenderPlan *renderPlan, DisplayPipelineType *pipelineType) {
    nextScreenState.data.currentTime = appState->sharedState.environmentState.currentTime;
    nextScreenState.data.temperatureC = appState->sharedState.environmentState.temperatureC;
    nextScreenState.data.relativeHumidity = appState->sharedState.environmentState.relativeHumidity;
    nextScreenState.data.batteryLevel = appState->sharedState.environmentState.batteryLevel;

    derivedStateFromAppState(appState);
    determineDirtyRegions();

    buildDisplayRenderPlan(appState, renderPlan);
    *pipelineType = DISPLAY_PIPELINE_TYPE_MONO;

    homeScreenState = nextScreenState;

    if (renderPlan->count == 0) {
        ESP_LOGI(TAG, "No changes detected in display data, skipping render");
        return DISPLAY_PREPARE_REQUEST_SKIPPED;
    }

    return DISPLAY_PREPARE_REQUEST_READY;
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

    snprintf(
        nextScreenState.derived.batteryLevelText,
        sizeof(nextScreenState.derived.batteryLevelText),
        "%3d%%",
        appState->sharedState.environmentState.batteryLevel
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

    if(homeScreenState.data.batteryLevel != nextScreenState.data.batteryLevel) {
        dirtyDisplayRegions[HOME_REGION_SLOT_BATTERY].isDirty = strcmp(nextScreenState.derived.batteryLevelText, homeScreenState.derived.batteryLevelText) != 0;
    }
}

static void buildDisplayRenderPlan(const AppState *appState, DisplayRenderPlan *displayRenderPlan) {
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

        displayRenderPlan->regions[sceneItemIndex++] = clockScene;
    }

    if (dirtyDisplayRegions[HOME_REGION_SLOT_BATTERY].isDirty) {
        RenderRegionScene batteryScene = {
            .regionId = DISPLAY_REGION_BATTERY,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_BATTERY].pixelRegion,
            .renderItems = {
                [0] = createBatteryLevelRenderItem(appState),
                [1] = createBatteryIconRenderItem(appState)
            },
            .count = 2
        };
        displayRenderPlan->regions[sceneItemIndex++] = batteryScene;
    }

    if(dirtyDisplayRegions[HOME_REGION_SLOT_TEMPERATURE].isDirty) {
        RenderRegionScene temperatureScene = {
            .regionId = DISPLAY_REGION_TEMPERATURE,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_TEMPERATURE].pixelRegion,
            .renderItems = {
                [0] = createTemperatureRenderItem(appState),
                [1] = createTemperatureIconRenderItem(appState)
            },
            .count = 2
        };
        displayRenderPlan->regions[sceneItemIndex++] = temperatureScene;
    }

    if(dirtyDisplayRegions[HOME_REGION_SLOT_HUMIDITY].isDirty) {
        RenderRegionScene humidityScene = {
            .regionId = DISPLAY_REGION_HUMIDITY,
            .pixelRegion = displayRegions[HOME_REGION_SLOT_HUMIDITY].pixelRegion,
            .renderItems = {
                [0] = createHumidityRenderItem(appState),
                [1] = createHumidityIconRenderItem(appState)
            },
            .count = 2
        };
        displayRenderPlan->regions[sceneItemIndex++] = humidityScene;
    }

    displayRenderPlan->count = sceneItemIndex;
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
    clockTextPosition.x -= 8; // Padding from the right edge of the region
    clockTextPosition.y += 8; // Padding from the top edge of the region

    PixelRenderItem renderItem = createTextRenderItem(clockTextPosition, nextScreenState.derived.clockText, &Font18);

    return renderItem;
}


static PixelRenderItem createBatteryLevelRenderItem(const AppState *appState) {        
    ESP_LOGI(TAG, "Battery level changed from '%s' to '%s'", homeScreenState.derived.batteryLevelText, nextScreenState.derived.batteryLevelText);

    struct PixelCoordinates2D batteryLevelTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_BATTERY], nextScreenState.derived.batteryLevelText, &Font12, REGION_ALIGNMENT_TOP_LEFT);
    batteryLevelTextPosition.x += 48; // Adjust text position to be right of the battery icon
    batteryLevelTextPosition.y += 8; // Padding from the top edge of the region
    PixelRenderItem renderItem = createTextRenderItem(batteryLevelTextPosition, nextScreenState.derived.batteryLevelText, &Font12);

    return renderItem;
}

static PixelRenderItem createTemperatureRenderItem(const AppState *appState) {
    ESP_LOGI(TAG, "Temperature text changed from '%s' to '%s'", homeScreenState.derived.temperatureText, nextScreenState.derived.temperatureText);

    struct PixelCoordinates2D temperatureTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_TEMPERATURE], nextScreenState.derived.temperatureText, &Font48, REGION_ALIGNMENT_CENTER);
    PixelRenderItem renderItem = createTextRenderItem(temperatureTextPosition, nextScreenState.derived.temperatureText, &Font48);

    return renderItem;
}

static PixelRenderItem createTemperatureIconRenderItem(const AppState *appState) {
    struct PixelCoordinates2D iconPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_TEMPERATURE], nextScreenState.derived.temperatureText, &Font48, REGION_ALIGNMENT_CENTER);
    iconPosition.x -= 48; // Adjust icon position to be left of the temperature text
    iconPosition.y += 16; // Adjust icon position to be vertically centered with the temperature text

    PixelRenderItem renderItem = createIconBitmapRenderItem(iconPosition, selectTemperatureIcon(nextScreenState.data.temperatureC), DISPLAY_COLOR_BLACK);

    return renderItem;
}

static PixelRenderItem createHumidityIconRenderItem(const AppState *appState) {
    struct PixelCoordinates2D iconPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_HUMIDITY], nextScreenState.derived.humidityText, &Font16, REGION_ALIGNMENT_TOP_CENTER);
    iconPosition.x -= 18; // Adjust icon position to be left of the humidity text
    iconPosition.y += 4; // Adjust icon position to be vertically centered with the humidity text

    PixelRenderItem renderItem = createIconBitmapRenderItem(iconPosition, ICON_TINT_E80B_24, DISPLAY_COLOR_BLACK);

    return renderItem;
}

static PixelRenderItem createHumidityRenderItem(const AppState *appState) {
    ESP_LOGI(TAG, "Humidity text changed from '%s' to '%s'", homeScreenState.derived.humidityText, nextScreenState.derived.humidityText);

    struct PixelCoordinates2D humidityTextPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_HUMIDITY], nextScreenState.derived.humidityText, &Font16, REGION_ALIGNMENT_TOP_CENTER);
    PixelRenderItem renderItem = createTextRenderItem(humidityTextPosition, nextScreenState.derived.humidityText, &Font16);

    return renderItem;
}

static PixelRenderItem createBatteryIconRenderItem(const AppState *appState) {
    ESP_LOGI(TAG, "Battery icon changed from '%s' to '%s'", homeScreenState.derived.batteryLevelText, nextScreenState.derived.batteryLevelText);

    struct PixelCoordinates2D batteryIconPosition = calculateAlignedTextPosition(&displayRegions[HOME_REGION_SLOT_BATTERY], nextScreenState.derived.batteryLevelText, &Font12, REGION_ALIGNMENT_TOP_LEFT);
    batteryIconPosition.x += 8; // Padding from the left edge of the region
    batteryIconPosition.y += 8; // Padding from the top edge of the region

    PixelRenderItem renderItem = createIconBitmapRenderItem(batteryIconPosition, selectBatteryIcon(nextScreenState.data.batteryLevel), DISPLAY_COLOR_BLACK);

    return renderItem;
}

static IconId selectTemperatureIcon(float temperatureC) {
    if (temperatureC <= 5.0) {
        ESP_LOGI(TAG, "Temperature icon selected for temperature: %f, Very Low", temperatureC);
        return ICON_THERMOMETER_0_F2CB_48;
    } else if (temperatureC <= 15.0) {
        ESP_LOGI(TAG, "Temperature icon selected for temperature: %f, Low", temperatureC);
        return ICON_THERMOMETER_QUARTER_F2CA_48;
    } else if (temperatureC <= 24.0) {
        ESP_LOGI(TAG, "Temperature icon selected for temperature: %f, Moderate", temperatureC);
        return ICON_THERMOMETER_2_F2C9_48;
    } else if (temperatureC <= 27.0) {
        ESP_LOGI(TAG, "Temperature icon selected for temperature: %f, High", temperatureC);
        return ICON_THERMOMETER_3_F2C8_48;
    } else {
        ESP_LOGI(TAG, "Temperature icon selected for temperature: %f, Very High", temperatureC);
        return ICON_THERMOMETER_F2C7_48;
    }
}

static IconId selectBatteryIcon(int batteryLevel) {
    if (batteryLevel >= 80) {
        ESP_LOGI(TAG, "Battery icon selected for battery level: %d, Full", batteryLevel);
        return ICON_BATTERY_4_F240_24;
    } else if (batteryLevel >= 60) {
        ESP_LOGI(TAG, "Battery icon selected for battery level: %d, High", batteryLevel);
        return ICON_BATTERY_3_F241_24;
    } else if (batteryLevel >= 40) {
        ESP_LOGI(TAG, "Battery icon selected for battery level: %d, Medium", batteryLevel);
        return ICON_BATTERY_2_F242_24;
    } else if (batteryLevel >= 20) {
        ESP_LOGI(TAG, "Battery icon selected for battery level: %d, Low", batteryLevel);
        return ICON_BATTERY_1_F243_24;
    } else {
        ESP_LOGI(TAG, "Battery icon selected for battery level: %d, Very Low", batteryLevel);
        return ICON_BATTERY_0_F244_24;
    }
}

static void initRenderRegions(void)
{
    displayRegions[HOME_REGION_SLOT_CLOCK].gridRegion = (struct GridRegion){ .x = 2, .y = 0, .width = 3, .height = 1 };
    displayRegions[HOME_REGION_SLOT_TEMPERATURE].gridRegion = (struct GridRegion){ .x = 1, .y = 1, .width = 3, .height = 2 };
    displayRegions[HOME_REGION_SLOT_HUMIDITY].gridRegion = (struct GridRegion){ .x = 1, .y = 3, .width = 3, .height = 1 };
    displayRegions[HOME_REGION_SLOT_ALERT].gridRegion = (struct GridRegion){.x = 1, .y = 1, .width = 1, .height = 2};
    displayRegions[HOME_REGION_SLOT_BATTERY].gridRegion = (struct GridRegion){.x = 0, .y = 0, .width = 2, .height = 1 };

    const int regionCount = ARRAY_SIZE(displayRegions);
    calculateDisplayRegionsPixelSpace(displayRegions, regionCount, screenLayout);
}

static void deinitDisplay(void) {
    homeScreenState = (HomeScreenState){0};
    nextScreenState = (HomeScreenState){0};

    ESP_LOGI(TAG, "Display deinitialized and state reset");
}