#pragma once
#include <pebble.h>  

#ifdef PBL_COLOR  

void flip_layer_color_image(GBitmap *img);
  
#endif  
  
// colors
GColor digit_back, digit_img;  
GColor color_back, color_date, color_dow, color_battery, color_bluetooth;  