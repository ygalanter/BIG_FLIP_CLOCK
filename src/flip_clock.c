#include <pebble.h>
#include "flip_layer.h"

static Window *window;
static FlipLayer *layer[4];

static TextLayer *text_layer_date;
char buffer_date[] = "SEP 31";

static TextLayer *text_layer_dow;
char buffer_dow[] = "SAT   ";

static Layer *batteryLayer;


static void batteryLayer_update_callback(Layer *me, GContext* ctx) {
	GRect layer_bounds = layer_get_bounds(me);
  BatteryChargeState state =  battery_state_service_peek();

  graphics_context_set_fill_color(ctx, GColorBlack);
  
  if (state.is_charging) {
     graphics_fill_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100 , layer_bounds.size.h), 0, 0);  
  } else {
     graphics_draw_rect(ctx, GRect(0, 0, layer_bounds.size.w * state.charge_percent / 100 , layer_bounds.size.h));  
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
   
  text_layer_date = text_layer_create(GRect(0, -4, 144, 60));
  text_layer_set_text_color(text_layer_date, GColorBlack);
  text_layer_set_text_alignment(text_layer_date, GTextAlignmentCenter);
  text_layer_set_font(text_layer_date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_date));
  
  text_layer_dow = text_layer_create(GRect(0, 106, 144, 60));
  text_layer_set_text_color(text_layer_dow, GColorBlack);
  text_layer_set_text_alignment(text_layer_dow, GTextAlignmentCenter);
  text_layer_set_font(text_layer_dow, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_dow));

  for(int i=0; i<4; i++){
    layer[i] = flip_layer_create(GRect(36 * i + (i>1? 1:0) , (168-60)/2, 35, 60));
    layer_add_child(window_layer, flip_layer_get_layer(layer[i]));
  }
  
  batteryLayer = layer_create(GRect(2, 2, 140, 3));
	layer_set_update_proc(batteryLayer, batteryLayer_update_callback);
  layer_add_child(window_layer, batteryLayer);
    
}

static void window_unload(Window *window) {
  for(int i=0; i<4; i++){
    flip_layer_destroy(layer[i]);
  }
  
  text_layer_destroy(text_layer_date);
  text_layer_destroy(text_layer_dow);
  layer_destroy(batteryLayer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_background_color(window, GColorWhite);
  window_stack_push(window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
