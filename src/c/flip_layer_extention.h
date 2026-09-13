#pragma once
#include <pebble.h>

#ifdef PBL_COLOR

// Colors digit image (replaces white with digit_img, black with digit_back)
void flip_layer_color_image(GBitmap *img);

#endif

// configurable colors (driven by Clay) - defined in flip_clock.c.
// On color platforms all four are honored; on B&W the tile colors (digit_back/
// digit_img) stay fixed at black/white since 1-bit tiles cannot be recolored.
extern GColor digit_back, digit_img;
extern GColor color_back, color_date, color_dow, color_battery, color_bluetooth;
