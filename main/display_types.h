#include "epaper_port.h"

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
