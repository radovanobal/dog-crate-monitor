#ifndef DOG_CRATE_MONITOR_DISPLAY_TYPES_H
#define DOG_CRATE_MONITOR_DISPLAY_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "GUI_Paint.h"
#include "generated_icons.h"

#define MAX_RENDER_ITEMS_PER_REGION 4
#define MAX_RENDER_SCENES 8
#define MAX_GRID_CELLS 70

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

typedef uint16_t DisplayRegionId;

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
    RENDER_ITEM_TYPE_CLEAR = 0,
    RENDER_ITEM_TYPE_TEXT = 1,
    RENDER_ITEM_TYPE_BITMAP = 2,
    RENDER_ITEM_TYPE_RECT = 3,
    RENDER_ITEM_TYPE_LINE = 4,
    RENDER_ITEM_TYPE_ICON = 5,
    RENDER_ITEM_TYPE_GRID = 6
} RenderItemType;

typedef enum {
    DISPLAY_SUCCESS = 0,
    DISPLAY_WARNING = 1,
    DISPLAY_FAIL = -1
} display_init_error;

typedef enum {
    DISPLAY_PIPELINE_TYPE_MONO = 0,
    DISPLAY_PIPELINE_TYPE_GRAYSCALE = 1
} DisplayPipelineType;

typedef enum {
    DISPLAY_COLOR_BLACK = 0,
    DISPLAY_COLOR_GRAY1 = 1,
    DISPLAY_COLOR_GRAY2 = 2,
    DISPLAY_COLOR_WHITE = 3
} DisplayColor;

typedef struct {
    uint8_t columnSpan;
    bool isActive;
    char text[16];
} BoxGridCellData;

typedef struct {
    struct PixelCoordinates2D position;
    struct PixelSize2D size;
    BoxGridCellData cells[MAX_GRID_CELLS];
    uint16_t colorAccent;
    uint16_t colorDefault;
    uint8_t rows;
    uint8_t columns;
} BoxGridData;

typedef struct {
    RenderItemType type;
    union {
        struct {
            struct PixelCoordinates2D position;
            char text[64];
            sFONT *font;
        } text;
        struct {
            struct PixelCoordinates2D position;
            const unsigned char *imageData;
            struct PixelSize2D size;
        } bitmap;
        struct {
            struct PixelCoordinates2D position;
            IconId iconId;
            DisplayColor color;
        } icon;
        struct {
            struct PixelCoordinates2D position;
            struct PixelSize2D size;
            DisplayColor color;
            DOT_PIXEL thickness;
            DRAW_FILL fillType;
        } rect;
        struct {
            struct PixelCoordinates2D start;
            struct PixelCoordinates2D end;
            DisplayColor color;
            DOT_PIXEL thickness;
            LINE_STYLE style;
        } line;
        BoxGridData grid;
    } data;
} PixelRenderItem;

typedef struct {
    DisplayRegionId regionId;
    PixelRegion pixelRegion;
    PixelRenderItem renderItems[MAX_RENDER_ITEMS_PER_REGION];
    size_t count;
} RenderRegionScene;

typedef struct {
    RenderRegionScene regions[MAX_RENDER_SCENES];
    size_t count;
} DisplayRenderPlan;

typedef struct {
    display_init_error (*init)(void);
    void (*deinit)(void);
    void (*prepareBuffer)(const DisplayRenderPlan *displayRenderPlan, const DisplayPaintType displayPaintType);
    void (*flushBufferToDisplay)(const DisplayPaintType displayPaintType);
    uint16_t (*getColor)(DisplayColor color);
    DisplayPipelineType pipelineType;
} DisplayPipelineInterface;

#endif // DOG_CRATE_MONITOR_DISPLAY_TYPES_H
