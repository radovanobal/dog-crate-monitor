#ifndef DOG_CRATE_MONITOR_DISPLAY_TYPES_H
#define DOG_CRATE_MONITOR_DISPLAY_TYPES_H

#include <stdint.h>
#include <stdbool.h>

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
    uint16_t x;
    uint16_t y;
};

typedef struct {
    char temperatureText[16];
    char humidityText[16];
    char clockText[16];
    bool showEnvironmentWarning;
    bool showBluetooth;
    bool showWifi;
    bool showBattery;
}DisplayState;

typedef enum {
    DISPLAY_REGION_CLOCK = 0,
    DISPLAY_REGION_TEMPERATURE = 1,
    DISPLAY_REGION_HUMIDITY = 2,
    DISPLAY_REGION_ALERT = 3,
}DisplayRegionId;

typedef struct {
    DisplayRegionId id;
    struct GridRegion gridRegion;
    struct PixelRegion pixelRegion;
}DisplayRegionDescriptor;


#endif // DOG_CRATE_MONITOR_DISPLAY_TYPES_H
