#pragma once
#include <pebble.h>  
  
#ifdef PBL_COLOR  

// Colors digit image (replaces white with digit_back, black with digit_img)  
void flip_layer_color_image(GBitmap *img);
  
#endif  

// colors
GColor digit_back, digit_img;  
GColor color_back, color_date, color_dow, color_battery, color_bluetooth;    

