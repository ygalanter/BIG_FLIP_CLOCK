#include <pebble.h>
#include "flip_layer_extention.h"
#include "gbitmap_color_palette_manipulator.h"

/* Extending flip-layer with color manipulation & other functions */    
  
#ifdef PBL_COLOR  

void flip_layer_color_image(GBitmap *img) {
  replace_gbitmap_color(GColorBlack, digit_back, img, NULL); 
  replace_gbitmap_color(GColorWhite, digit_img, img, NULL); 
}

#endif