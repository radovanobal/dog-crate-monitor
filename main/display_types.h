#ifndef DOG_CRATE_MONITOR_DISPLAY_TYPES_H
#define DOG_CRATE_MONITOR_DISPLAY_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "GUI_Paint.h"

#define MAX_RENDER_ITEMS 12

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

typedef struct {
    int x;
    int y;
    int width;
    int height;
} PixelRegion;

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
} DisplayState;

typedef enum {
    DISPLAY_REGION_CLOCK = 0,
    DISPLAY_REGION_TEMPERATURE = 1,
    DISPLAY_REGION_HUMIDITY = 2,
    DISPLAY_REGION_ALERT = 3,
} DisplayRegionId;

typedef struct {
    DisplayRegionId id;
    struct GridRegion gridRegion;
    PixelRegion pixelRegion;
} DisplayRegionDescriptor;

typedef enum {
    DISPLAY_PAINT_TYPE_NONE = 0,
    DISPLAY_PAINT_TYPE_FULL = 1,
    DISPLAY_PAINT_TYPE_PARTIAL = 2
} DisplayPaintType;

typedef enum {
    RENDER_ITEM_TYPE_TEXT = 0,
    RENDER_ITEM_TYPE_BITMAP,
    RENDER_ITEM_TYPE_RECT,
    RENDER_ITEM_TYPE_LINE
} RenderItemType;

typedef struct {
    RenderItemType type;
    PixelRegion pixelRegion;
    union {
        struct {
            struct PixelCoordinates2D position;
            char text[32];
            sFONT *font;
        } text;
        struct {
            struct PixelCoordinates2D position;
            const unsigned char *imageData;
            struct PixelSize2D size;
        } bitmap;
        struct {
            struct PixelCoordinates2D position;
            struct PixelSize2D size;
            uint16_t color;
            DOT_PIXEL thickness;
            DRAW_FILL fillType;
        } rect;
        struct {
            struct PixelCoordinates2D start;
            struct PixelCoordinates2D end;
            uint16_t color;
            DOT_PIXEL thickness;
            LINE_STYLE style;
        } line;
    } data;
} PixelRenderItem;

typedef struct {
    PixelRenderItem items[MAX_RENDER_ITEMS];
    size_t count;
} DisplayRenderPlan;

typedef enum {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
} display_init_error;

#endif // DOG_CRATE_MONITOR_DISPLAY_TYPES_H
