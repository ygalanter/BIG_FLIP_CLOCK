#include <pebble.h>
#include "flip_layer.h"
#include "flip_layer_extention.h"


#define NUMBER_IMAGE_COUNT 10

int NUMBER_IMAGE_RESOURCE_UP_IDS[NUMBER_IMAGE_COUNT] = {
  RESOURCE_ID_IMAGE_0_UP,
  RESOURCE_ID_IMAGE_1_UP,
  RESOURCE_ID_IMAGE_2_UP,
  RESOURCE_ID_IMAGE_3_UP,
  RESOURCE_ID_IMAGE_4_UP,
  RESOURCE_ID_IMAGE_5_UP,
  RESOURCE_ID_IMAGE_6_UP,
  RESOURCE_ID_IMAGE_7_UP,
  RESOURCE_ID_IMAGE_8_UP,
  RESOURCE_ID_IMAGE_9_UP,
};

int NUMBER_IMAGE_RESOURCE_DOWN_IDS[NUMBER_IMAGE_COUNT] = {
  RESOURCE_ID_IMAGE_0_DOWN,
  RESOURCE_ID_IMAGE_1_DOWN,
  RESOURCE_ID_IMAGE_2_DOWN,
  RESOURCE_ID_IMAGE_3_DOWN,
  RESOURCE_ID_IMAGE_4_DOWN,
  RESOURCE_ID_IMAGE_5_DOWN,
  RESOURCE_ID_IMAGE_6_DOWN,
  RESOURCE_ID_IMAGE_7_DOWN,
  RESOURCE_ID_IMAGE_8_DOWN,
  RESOURCE_ID_IMAGE_9_DOWN,
};


// --- Per-platform digit tile geometry ----------------------------------------
// The tile PNGs ship in three sizes (base 35x30, emery 47x40, gabbro 58x50),
// wired per platform in package.json. These constants must match the bundled art.
#if PBL_DISPLAY_WIDTH >= 260        // gabbro (260x260 round)
  #define DIGIT_W 58
  #define DIGIT_H 50
  #define DIGIT_GAP 1
  #define DIGIT_MID_GAP 5
#elif PBL_DISPLAY_WIDTH >= 200      // emery (200x228 rect)
  #define DIGIT_W 47
  #define DIGIT_H 40
  #define DIGIT_GAP 1
  #define DIGIT_MID_GAP 4
#elif PBL_DISPLAY_WIDTH >= 180      // chalk (180x180 round)
  #define DIGIT_W 37
  #define DIGIT_H 30
  #define DIGIT_GAP 3
  #define DIGIT_MID_GAP 3
#else                               // 144x168 (aplite/basalt/diorite/flint)
  #define DIGIT_W 35
  #define DIGIT_H 30
  #define DIGIT_GAP 1
  #define DIGIT_MID_GAP 1
#endif

#define DIGIT_BLOCK_H (2 * DIGIT_H)
#define DIGIT_BLOCK_W (4 * DIGIT_W + 3 * DIGIT_GAP + DIGIT_MID_GAP)

// Date / day-of-week font + placement. On the large screens (emery, gabbro) use a
// bigger digital-7 and hug the two rows to the centered clock block. The GLYPH
// offsets are the measured inset of the rendered glyph within the text layer.
#if PBL_DISPLAY_WIDTH >= 200            // emery & gabbro (large)
  #define DATE_FONT_ID   RESOURCE_ID_DIGITAL_SEVEN_MONO_64
  #define DATE_LAYER_H   64
  #define DATE_GLYPH_TOP 22
  #define DATE_GLYPH_BOT 63
  #define DATE_BOT_GAP   16
  #define LARGE_DATE 1
#else                                  // 144 rect / chalk round (original placement)
  #define DATE_FONT_ID PBL_IF_RECT_ELSE(RESOURCE_ID_DIGITAL_SEVEN_MONO_50, RESOURCE_ID_DIGITAL_SEVEN_MONO_40)
  #define DATE_LAYER_H 60
  #define LARGE_DATE 0
#endif


static Window *window;
static FlipLayer *layer[4];

static TextLayer *text_layer_date;
static char buffer_date[16] = "SEP 31";

static TextLayer *text_layer_dow;
static char buffer_dow[16] = "SAT";

static Layer *batteryLayer;
static Layer *bluetoothLayer;
#ifdef PBL_ROUND
static BitmapLayer *ptr_bg_layer;
#endif

static GRect bounds;

int DateYcoord;
int DoWYcoord;

// definitions for the color globals declared extern in flip_layer_extention.h
GColor digit_back, digit_img;
GColor color_back, color_date, color_dow, color_battery, color_bluetooth;

// {*** Begin configurable option

// AppMessage keys are the auto-generated MESSAGE_KEY_* externs (from package.json).
// Persistent-storage keys live in their own namespace, separate from message keys.
// The three toggle keys keep their historical persist ids so existing users' choices
// survive the update; the color keys are new.
#define PERSIST_SHOW_BATTERY  1
#define PERSIST_SWAP_DATE_DOW 2
#define PERSIST_ENABLE_BT     3
#define PERSIST_COLOR_BG      10
#define PERSIST_COLOR_TIME_BG 11
#define PERSIST_COLOR_TIME    12
#define PERSIST_COLOR_DATE    13


static bool persist_bool_or(uint32_t key, bool def) {
  return persist_exists(key) ? persist_read_bool(key) : def;
}

static GRect battery_rect(void) {
  // Uniform across all platforms: a thin bar just above the time block, aligned to
  // the digit-block width (so it stays inside the circle on round screens too).
  int block_y = bounds.origin.y + (bounds.size.h - DIGIT_BLOCK_H) / 2;
  int bx = bounds.origin.x + (bounds.size.w - DIGIT_BLOCK_W) / 2;
  return GRect(bx + 4, block_y - 6, DIGIT_BLOCK_W - 8, 3);
}

static GRect bluetooth_rect(void) {
  // Mirror of the battery bar, just below the time block (same width/alignment).
  int block_y = bounds.origin.y + (bounds.size.h - DIGIT_BLOCK_H) / 2;
  int bx = bounds.origin.x + (bounds.size.w - DIGIT_BLOCK_W) / 2;
  return GRect(bx + 4, block_y + DIGIT_BLOCK_H + 3, DIGIT_BLOCK_W - 8, 3);
}

static void bluetoothLayer_update_callback(Layer *me, GContext* ctx) {
  GRect b = layer_get_bounds(me);
  graphics_context_set_fill_color(ctx, color_bluetooth);
  graphics_fill_rect(ctx, GRect(0, 0, b.size.w, b.size.h), 0, 0);
}

// The BT line is shown only when the alert is enabled AND the phone is connected.
static void update_bt_line(void) {
  bool visible = persist_bool_or(PERSIST_ENABLE_BT, true) &&
                 connection_service_peek_pebble_app_connection();
  layer_set_frame(bluetoothLayer, visible ? bluetooth_rect() : GRect(0, 0, 0, 0));
  layer_mark_dirty(bluetoothLayer);
}

static void set_date_dow_coords(bool swapped) {
#if LARGE_DATE
  // hug the two text rows to the vertically-centered clock block, using the measured
  // glyph insets: the date sits above the battery bar (~6px above the block) with an
  // 8px gap; the DoW sits below the block by DATE_BOT_GAP.
  int block_y = bounds.origin.y + (bounds.size.h - DIGIT_BLOCK_H) / 2;
  int top_y = block_y - 6 - 8 - DATE_GLYPH_BOT;
  int bot_y = block_y + DIGIT_BLOCK_H + DATE_BOT_GAP - DATE_GLYPH_TOP;
#else
  int top_y = bounds.origin.y + PBL_IF_RECT_ELSE(-4, 10);
  int bot_y = bounds.size.h - 62;
#endif
  if (swapped) {
    DateYcoord = bot_y;
    DoWYcoord = top_y;
  } else {
    DateYcoord = top_y;
    DoWYcoord = bot_y;
  }
}

static void batteryLayer_update_callback(Layer *me, GContext* ctx) {
  GRect layer_bounds = layer_get_bounds(me);
  BatteryChargeState state = battery_state_service_peek();

  if (state.is_charging) {
    graphics_context_set_fill_color(ctx, color_battery);
    graphics_fill_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100, layer_bounds.size.h), 0, 0);
  } else {
    graphics_context_set_stroke_color(ctx, color_battery);
    graphics_draw_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100, layer_bounds.size.h));
  }
}

// Clay sends toggles as ints; colors arrive as ints too. Accept a string form as well.
static int tuple_to_int(Tuple *t) {
  if (!t) return 0;
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  switch (t->length) {
    case 1: return t->value->uint8;
    case 2: return t->value->int16;
    default: return t->value->int32;
  }
}

// Recompute the derived colors/flags and repaint after a color change.
static void apply_colors(void) {
  color_dow = color_date;
  color_battery = color_date;
  color_bluetooth = color_date;

  window_set_background_color(window, color_back);
  text_layer_set_text_color(text_layer_date, color_date);
  text_layer_set_text_color(text_layer_dow, color_dow);
#ifdef PBL_ROUND
  bitmap_layer_set_background_color(ptr_bg_layer, digit_img);
#endif
  for (int i = 0; i < 4; i++) {
    flip_layer_apply_colors(layer[i]);
  }
  layer_mark_dirty(batteryLayer);
  layer_mark_dirty(bluetoothLayer);
}

static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  Tuple *t;
  bool colors_changed = false;

  t = dict_find(iterator, MESSAGE_KEY_COLOR_BG);
  if (t) { int v = tuple_to_int(t); color_back = GColorFromHEX(v); persist_write_int(PERSIST_COLOR_BG, v); colors_changed = true; }

  t = dict_find(iterator, MESSAGE_KEY_COLOR_TIME_BG);
  if (t) {
    persist_write_int(PERSIST_COLOR_TIME_BG, tuple_to_int(t));
#ifdef PBL_COLOR  // B&W tiles stay fixed (white digits on black tiles)
    digit_back = GColorFromHEX(tuple_to_int(t)); colors_changed = true;
#endif
  }

  t = dict_find(iterator, MESSAGE_KEY_COLOR_TIME);
  if (t) {
    persist_write_int(PERSIST_COLOR_TIME, tuple_to_int(t));
#ifdef PBL_COLOR
    digit_img = GColorFromHEX(tuple_to_int(t)); colors_changed = true;
#endif
  }

  t = dict_find(iterator, MESSAGE_KEY_COLOR_DATE);
  if (t) { int v = tuple_to_int(t); color_date = GColorFromHEX(v); persist_write_int(PERSIST_COLOR_DATE, v); colors_changed = true; }

  t = dict_find(iterator, MESSAGE_KEY_SHOW_BATTERY);
  if (t) {
    bool show = tuple_to_int(t) == 1;
    persist_write_bool(PERSIST_SHOW_BATTERY, show);
    layer_set_frame(batteryLayer, show ? battery_rect() : GRect(0, 0, 0, 0));
    layer_mark_dirty(batteryLayer);
  }

  t = dict_find(iterator, MESSAGE_KEY_SWAP_DATE_DOW);
  if (t) {
    bool swapped = tuple_to_int(t) == 1;
    persist_write_bool(PERSIST_SWAP_DATE_DOW, swapped);
    set_date_dow_coords(swapped);
    layer_set_frame(text_layer_get_layer(text_layer_date), GRect(0, DateYcoord, bounds.size.w, DATE_LAYER_H));
    layer_set_frame(text_layer_get_layer(text_layer_dow), GRect(0, DoWYcoord, bounds.size.w, DATE_LAYER_H));
  }

  t = dict_find(iterator, MESSAGE_KEY_ENABLE_BT_NOTIF);
  if (t) {
    persist_write_bool(PERSIST_ENABLE_BT, tuple_to_int(t) == 1);
    update_bt_line();
  }

  if (colors_changed) {
    apply_colors();
  }
}

// End configurable option ***}


// BT connected/disconnected: buzz (if the alert is enabled) and toggle the line.
void display_bt_layer(bool connected) {
  if (persist_bool_or(PERSIST_ENABLE_BT, true) == false) return;
  vibes_double_pulse();
  update_bt_line();
}


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {

  strftime(buffer_date, sizeof(buffer_date), "%b %d", tick_time);
  text_layer_set_text(text_layer_date, buffer_date);

  strftime(buffer_dow, sizeof(buffer_dow), "%a", tick_time);
  text_layer_set_text(text_layer_dow, buffer_dow);

  if (!clock_is_24h_style()) {
    if (tick_time->tm_hour > 11) {   // 0..11 - am 12..23 - pm
      strcat(buffer_dow, " PM");
      if (tick_time->tm_hour > 12) tick_time->tm_hour -= 12;
    } else {
      strcat(buffer_dow, " AM");
      if (tick_time->tm_hour == 0) tick_time->tm_hour = 12;
    }
  }

  flip_layer_animate_to(layer[0], tick_time->tm_hour / 10);
  flip_layer_animate_to(layer[1], tick_time->tm_hour % 10);
  flip_layer_animate_to(layer[2], tick_time->tm_min / 10);
  flip_layer_animate_to(layer[3], tick_time->tm_min % 10);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);

  set_date_dow_coords(persist_bool_or(PERSIST_SWAP_DATE_DOW, false));

  int block_y = bounds.origin.y + (bounds.size.h - DIGIT_BLOCK_H) / 2;
  int start_x = bounds.origin.x + (bounds.size.w - DIGIT_BLOCK_W) / 2;

#ifdef PBL_ROUND
  ptr_bg_layer = bitmap_layer_create(GRect(0, block_y, bounds.size.w, DIGIT_BLOCK_H));
  bitmap_layer_set_background_color(ptr_bg_layer, digit_img);
  layer_add_child(window_layer, bitmap_layer_get_layer(ptr_bg_layer));
#endif

  for (int i = 0; i < 4; i++) {
    int x = start_x + i * (DIGIT_W + DIGIT_GAP) + (i >= 2 ? DIGIT_MID_GAP : 0);
    layer[i] = flip_layer_create(GRect(x, block_y, DIGIT_W, DIGIT_BLOCK_H));
  }

  for (int i = 0; i < 4; i++) {
    flip_layer_set_images(layer[i], NUMBER_IMAGE_RESOURCE_UP_IDS, NUMBER_IMAGE_RESOURCE_DOWN_IDS, NUMBER_IMAGE_COUNT);
    layer_add_child(window_layer, flip_layer_get_layer(layer[i]));
  }

  text_layer_date = text_layer_create(GRect(0, DateYcoord, bounds.size.w, DATE_LAYER_H));
  text_layer_set_text_color(text_layer_date, color_date);
  text_layer_set_background_color(text_layer_date, GColorClear);
  text_layer_set_text_alignment(text_layer_date, GTextAlignmentCenter);
  text_layer_set_font(text_layer_date, fonts_load_custom_font(resource_get_handle(DATE_FONT_ID)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_date));

  text_layer_dow = text_layer_create(GRect(0, DoWYcoord, bounds.size.w, DATE_LAYER_H));
  text_layer_set_text_color(text_layer_dow, color_dow);
  text_layer_set_background_color(text_layer_dow, GColorClear);
  text_layer_set_text_alignment(text_layer_dow, GTextAlignmentCenter);
  text_layer_set_font(text_layer_dow, fonts_load_custom_font(resource_get_handle(DATE_FONT_ID)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_dow));

  batteryLayer = layer_create(persist_bool_or(PERSIST_SHOW_BATTERY, true) ? battery_rect() : GRect(0, 0, 0, 0));
  layer_set_update_proc(batteryLayer, batteryLayer_update_callback);
  layer_add_child(window_layer, batteryLayer);

  // Bluetooth line: visible only when the alert is on and the phone is connected.
  bluetoothLayer = layer_create(GRect(0, 0, 0, 0));
  layer_set_update_proc(bluetoothLayer, bluetoothLayer_update_callback);
  layer_add_child(window_layer, bluetoothLayer);
  update_bt_line();
}

static void window_unload(Window *window) {
  for (int i = 0; i < 4; i++) {
    flip_layer_destroy(layer[i]);
  }

  text_layer_destroy(text_layer_date);
  text_layer_destroy(text_layer_dow);
#ifdef PBL_ROUND
  bitmap_layer_destroy(ptr_bg_layer);
#endif
  layer_destroy(batteryLayer);
  layer_destroy(bluetoothLayer);
}

static void load_colors(void) {
  // per-platform defaults (Clay's pickers auto-limit to black/white on B&W)
#ifdef PBL_COLOR
  color_back = GColorOxfordBlue;
  color_date = GColorChromeYellow;
  digit_back = GColorCeleste;
  digit_img  = GColorDarkGreen;
  if (persist_exists(PERSIST_COLOR_BG))      color_back = GColorFromHEX(persist_read_int(PERSIST_COLOR_BG));
  if (persist_exists(PERSIST_COLOR_TIME_BG)) digit_back = GColorFromHEX(persist_read_int(PERSIST_COLOR_TIME_BG));
  if (persist_exists(PERSIST_COLOR_TIME))    digit_img  = GColorFromHEX(persist_read_int(PERSIST_COLOR_TIME));
  if (persist_exists(PERSIST_COLOR_DATE))    color_date = GColorFromHEX(persist_read_int(PERSIST_COLOR_DATE));
#else
  // B&W: the 1-bit tile art cannot be recolored, so tiles stay as drawn
  // (white digits on black tiles). Only Background and Date are configurable.
  color_back = GColorWhite;
  color_date = GColorBlack;
  digit_back = GColorBlack;
  digit_img  = GColorWhite;

  if (persist_exists(PERSIST_COLOR_BG))   color_back = GColorFromHEX(persist_read_int(PERSIST_COLOR_BG));
  if (persist_exists(PERSIST_COLOR_DATE)) color_date = GColorFromHEX(persist_read_int(PERSIST_COLOR_DATE));
#endif

  color_dow = color_date;
  color_battery = color_date;
  color_bluetooth = color_date;
}

static void init(void) {
  setlocale(LC_ALL, "");

  load_colors();

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_set_background_color(window, color_back);
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(128, 64);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = display_bt_layer
  });
}

static void deinit(void) {
  app_message_deregister_callbacks();
  connection_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
