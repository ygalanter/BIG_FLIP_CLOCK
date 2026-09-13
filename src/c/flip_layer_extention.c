#include <pebble.h>
#include "flip_layer_extention.h"
#include "gbitmap_color_palette_manipulator.h"

/* Extending flip-layer with color manipulation & other functions */    
  
#ifdef PBL_COLOR

// Recolor the tile art: original black -> Time Background, original white -> Time.
// Single pass over the palette keyed on each entry's ORIGINAL value. A sequential
// two-call replace collides when Time Background is white: the black->white step
// then gets clobbered by the white->Time step, collapsing tile and digit to one
// color. Mapping both in one pass avoids that.
void flip_layer_color_image(GBitmap *img) {
  GColor *pal = gbitmap_get_palette(img);
  if (!pal) return;

  int n = 0;
  switch (gbitmap_get_format(img)) {
    case GBitmapFormat1BitPalette: n = 2;  break;
    case GBitmapFormat2BitPalette: n = 4;  break;
    case GBitmapFormat4BitPalette: n = 16; break;
    default: return;
  }

  for (int i = 0; i < n; i++) {
    if ((pal[i].argb & 0x3F) == (GColorBlack.argb & 0x3F)) {
      pal[i].argb = (pal[i].argb & 0xC0) | (digit_back.argb & 0x3F);
    } else if ((pal[i].argb & 0x3F) == (GColorWhite.argb & 0x3F)) {
      pal[i].argb = (pal[i].argb & 0xC0) | (digit_img.argb & 0x3F);
    }
  }
}

#endif