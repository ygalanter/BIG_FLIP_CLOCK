#include <pebble.h>
#include "flip_layer.h"
#include "flip_layer_extention.h"
#include "effec_layer.h"
  
  
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
  
  
static Window *window;
static FlipLayer *layer[4];

static TextLayer *text_layer_date;
char buffer_date[] = "SEP 31";

static TextLayer *text_layer_dow;
char buffer_dow[] = "SAT   ";

static Layer *batteryLayer;
static TextLayer *s_textlayer_bt;


// {*** Begin configurable option 
  
#define KEY_INVERT 0
#define KEY_SHOW_BATTERY 1
#define KEY_SWAP_DATE_DOW 2
#define KEY_ENABLE_BT_NOTIF 3  

static InverterLayer *inverter_layer;


#ifndef PBL_SDK_2
static void app_focus_changed(bool focused) {
  if (focused) { // on resuming focus - restore background
    layer_mark_dirty(effect_layer_get_layer(inverter_layer));
  }
}
#endif



static void batteryLayer_update_callback(Layer *me, GContext* ctx) {
	GRect layer_bounds = layer_get_bounds(me);
  BatteryChargeState state =  battery_state_service_peek();

    
  if (state.is_charging) {
     graphics_context_set_fill_color(ctx, color_battery);
     graphics_fill_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100 , layer_bounds.size.h), 0, 0);  
  } else {
     graphics_context_set_stroke_color(ctx, color_battery);
     graphics_draw_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100 , layer_bounds.size.h));  
  }
}


void position_battery_layer(int x, int y, int width, int height) {
  layer_set_frame(batteryLayer, GRect(x, y, width, height));
}

void position_inverter_layer(int x, int y, int width, int height) {
  layer_set_frame(inverter_layer_get_layer(inverter_layer), GRect(x, y, width, height));
}


int DateYcoord = -4;
int DoWYcoord = 106;

static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);

  while (t)  {
    switch(t->key)    {

      case KEY_INVERT:
        if (t->value->int8 == 1) {
          position_inverter_layer(0, 0, 144, 168);
          persist_write_bool(KEY_INVERT, true);
        } else {
          position_inverter_layer(0, 0, 0, 0);
          persist_write_bool(KEY_INVERT, false);  
        }
        break;
      
      case KEY_SHOW_BATTERY:
        if (t->value->int8 == 1) {
          position_battery_layer(2, 2, 140, 3);
          persist_write_bool(KEY_SHOW_BATTERY, true);
        } else {
          position_battery_layer(0, 0, 0, 0);
          persist_write_bool(KEY_SHOW_BATTERY, false);
        }      
        break;
      
      case KEY_SWAP_DATE_DOW:
      
        if (t->value->int8 == 1) {
          DateYcoord = 106;
          DoWYcoord = -4;
          persist_write_bool(KEY_SWAP_DATE_DOW, true);
        } else {
          DateYcoord = -4;
          DoWYcoord = 106; 
          persist_write_bool(KEY_SWAP_DATE_DOW, false);
        }
      
        layer_set_frame(text_layer_get_layer(text_layer_date), GRect(0, DateYcoord, 144, 60));
        layer_set_frame(text_layer_get_layer(text_layer_dow), GRect(0, DoWYcoord, 144, 60));
      
        break;
      
      case KEY_ENABLE_BT_NOTIF:
        
        if (t->value->int8 == 1) {
           persist_write_bool(KEY_ENABLE_BT_NOTIF, true);
        } else {
           persist_write_bool(KEY_ENABLE_BT_NOTIF, false);
        }
   
    }    
    
    t = dict_read_next(iterator);
  }
}



  
// End configurable option ***}


// show bt connected/disconnected
void display_bt_layer(bool connected) {
  
  // if bt notifs disabled - don't do anything
  if (persist_read_bool(KEY_ENABLE_BT_NOTIF) == false) return;
  
  vibes_double_pulse();
  if (connected) {
    text_layer_set_text_color(s_textlayer_bt, color_back);
  } else {
    text_layer_set_text_color(s_textlayer_bt, color_bluetooth);
  }
}


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  
  strftime(buffer_date, sizeof("SEP 31"), "%b %d", tick_time);
  text_layer_set_text(text_layer_date, buffer_date);
  
  strftime(buffer_dow, sizeof("SAT"), "%a", tick_time);
  text_layer_set_text(text_layer_dow, buffer_dow);
  
  if (!clock_is_24h_style()) {
    
      if( tick_time->tm_hour > 11 ) {   // YG Jun-25-2014: 0..11 - am 12..23 - pm
          strcat(buffer_dow, " PM" );
          if( tick_time->tm_hour > 12 ) tick_time->tm_hour -= 12;
      } else {
          strcat(buffer_dow, " AM" );
          if( tick_time->tm_hour == 0 ) tick_time->tm_hour = 12;
      }        
    
                    
  }
  
  flip_layer_animate_to(layer[0], tick_time->tm_hour / 10);
  flip_layer_animate_to(layer[1], tick_time->tm_hour % 10);
  flip_layer_animate_to(layer[2], tick_time->tm_min / 10);
  flip_layer_animate_to(layer[3], tick_time->tm_min % 10);
   
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  if (persist_read_bool(KEY_SWAP_DATE_DOW) == true) {
    DateYcoord = 106;
    DoWYcoord = -4;
  } else {
    DateYcoord = -4;
    DoWYcoord = 106;    
  }
   
  text_layer_date = text_layer_create(GRect(0, DateYcoord, 144, 60));
  text_layer_set_text_color(text_layer_date, color_date);
  text_layer_set_background_color(text_layer_date, GColorClear);
  text_layer_set_text_alignment(text_layer_date, GTextAlignmentCenter);
  text_layer_set_font(text_layer_date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_date));
  
  text_layer_dow = text_layer_create(GRect(0, DoWYcoord, 144, 60));
  text_layer_set_text_color(text_layer_dow, color_dow);
  text_layer_set_background_color(text_layer_dow, GColorClear);
  text_layer_set_text_alignment(text_layer_dow, GTextAlignmentCenter);
  text_layer_set_font(text_layer_dow, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_dow));
    
  // s_textlayer_bt
  s_textlayer_bt = text_layer_create(GRect(0, 152, 144, 16));
  text_layer_set_text(s_textlayer_bt, "Bluetooth disconnected");
  text_layer_set_background_color(s_textlayer_bt, GColorClear);
  text_layer_set_text_color(s_textlayer_bt, color_back);
  text_layer_set_text_alignment(s_textlayer_bt, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_bt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, (Layer *)s_textlayer_bt);
  

  for(int i=0; i<4; i++){
    layer[i] = flip_layer_create(GRect(36 * i + (i>1? 1:0) , (168-60)/2, 35, 60));
  }
  
  for(int i=0; i<4; i++){
    flip_layer_set_images(layer[i], NUMBER_IMAGE_RESOURCE_UP_IDS, NUMBER_IMAGE_RESOURCE_DOWN_IDS, NUMBER_IMAGE_COUNT);
    layer_add_child(window_layer, flip_layer_get_layer(layer[i]));
  }
  
  if (persist_read_bool(KEY_SHOW_BATTERY) == true) {
    batteryLayer = layer_create(GRect(2, 2, 140, 3));
  } else {
    batteryLayer = layer_create(GRect(0, 0, 0, 0));
  }
  layer_set_update_proc(batteryLayer, batteryLayer_update_callback);
  layer_add_child(window_get_root_layer(window), batteryLayer);
  
  if (persist_read_bool(KEY_INVERT) == true) {
    inverter_layer = inverter_layer_create(GRect(0, 0, 144,168));
  } else {
    inverter_layer = inverter_layer_create(GRect(0, 0, 0, 0));
  }
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
  
  // on initial load check BT
  if (persist_read_bool(KEY_ENABLE_BT_NOTIF) == true) {
      if (bluetooth_connection_service_peek()) {
        text_layer_set_text_color(s_textlayer_bt, color_back);
      } else {
        text_layer_set_text_color(s_textlayer_bt, color_bluetooth);
      }
  }
    
}

static void window_unload(Window *window) {
  for(int i=0; i<4; i++){
    flip_layer_destroy(layer[i]);
  }
  
  text_layer_destroy(text_layer_date);
  text_layer_destroy(text_layer_dow);
    
  inverter_layer_destroy(inverter_layer);
  layer_destroy(batteryLayer);
}

static void init(void) {
  
  

  #ifndef PBL_SDK_2
  // need to catch when app resumes focus after notification, otherwise background won't restore
  app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = app_focus_changed
  });
  #endif
  
  setlocale(LC_ALL, "");

// initializing colors
#ifdef PBL_COLOR
   color_back = GColorOxfordBlue;
   color_date = GColorChromeYellow;
   color_dow =  GColorChromeYellow;
   color_battery = GColorChromeYellow;
   color_bluetooth = GColorChromeYellow;
  
  digit_back = GColorCeleste ;
  digit_img = GColorDarkGreen;
  
#else
   color_back = GColorWhite;
   color_date = GColorBlack;
   color_dow =  GColorBlack;
   color_battery = GColorBlack;
   color_bluetooth = GColorBlack;
  
  digit_back = GColorBlack;
  digit_img = GColorWhite;
  
#endif
  
  
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  window_set_background_color(window, color_back);
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  bluetooth_connection_service_subscribe(display_bt_layer);
}

static void deinit(void) {
  
  #ifndef PBL_SDK_2
    app_focus_service_unsubscribe();
  #endif
  
  app_message_deregister_callbacks();
  bluetooth_connection_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
