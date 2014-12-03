#include <pebble.h>
#include "flip_layer.h"
  
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

static InverterLayer *inverter_layer;

void destroy_inveter_layer(){
    if (inverter_layer != NULL) {
      inverter_layer_destroy(inverter_layer);
      inverter_layer = NULL;
    }
}

void create_inverter_layer(){
  if (inverter_layer != NULL) destroy_inveter_layer(); 
  inverter_layer = inverter_layer_create(GRect(0, 0, 144,168));
  layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
}



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

void destroy_battery_layer() {
  if (batteryLayer != NULL) {
     layer_destroy(batteryLayer);
     batteryLayer = NULL;
  }
}


void create_battery_layer() {
  if (batteryLayer != NULL)  destroy_battery_layer();
  batteryLayer = layer_create(GRect(2, 2, 140, 3));
  layer_set_update_proc(batteryLayer, batteryLayer_update_callback);
  layer_add_child(window_get_root_layer(window), batteryLayer);
}

int DateYcoord = -4;
int DoWYcoord = 106;

static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);

  while (t)  {
    switch(t->key)    {

      case KEY_INVERT:
        if (t->value->int8 == 1) {
          create_inverter_layer();
          persist_write_bool(KEY_INVERT, true);
        } else {
          destroy_inveter_layer();
          persist_write_bool(KEY_INVERT, false);  
        }
        break;
      
      case KEY_SHOW_BATTERY:
        if (t->value->int8 == 1) {
          create_battery_layer();
          persist_write_bool(KEY_SHOW_BATTERY, true);
          //on initial load inverter layer needs to be recreated AFTER battery layer
          if (persist_read_bool(KEY_INVERT) == true) create_inverter_layer();
        } else {
          destroy_battery_layer();
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
   
    }    
    
    t = dict_read_next(iterator);
  }
}



  
// End configurable option ***}


// show bt connected/disconnected
void display_bt_layer(bool connected) {
  vibes_double_pulse();
  if (connected) {
    text_layer_set_text_color(s_textlayer_bt, GColorWhite);
  } else {
    text_layer_set_text_color(s_textlayer_bt, GColorBlack);
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
  text_layer_set_text_color(text_layer_date, GColorBlack);
  text_layer_set_text_alignment(text_layer_date, GTextAlignmentCenter);
  text_layer_set_font(text_layer_date, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_date));
  
  text_layer_dow = text_layer_create(GRect(0, DoWYcoord, 144, 60));
  text_layer_set_text_color(text_layer_dow, GColorBlack);
  text_layer_set_text_alignment(text_layer_dow, GTextAlignmentCenter);
  text_layer_set_font(text_layer_dow, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DIGITAL_SEVEN_MONO_50)));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_dow));
    
  // s_textlayer_bt
  s_textlayer_bt = text_layer_create(GRect(0, 152, 144, 16));
  text_layer_set_text(s_textlayer_bt, "Bluetooth disconnected");
  text_layer_set_background_color(s_textlayer_bt, GColorClear);
  text_layer_set_text_color(s_textlayer_bt, GColorClear);
  text_layer_set_text_alignment(s_textlayer_bt, GTextAlignmentCenter);
  text_layer_set_font(s_textlayer_bt, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_layer, (Layer *)s_textlayer_bt);
  

  for(int i=0; i<4; i++){
    layer[i] = flip_layer_create(GRect(36 * i + (i>1? 1:0) , (168-60)/2, 35, 60));
    layer_add_child(window_layer, flip_layer_get_layer(layer[i]));
  }
  
  if (persist_read_bool(KEY_SHOW_BATTERY) == true) {
    create_battery_layer();
  }
  
  if (persist_read_bool(KEY_INVERT) == true) {
    create_inverter_layer();   
  }
  
    
}

static void window_unload(Window *window) {
  for(int i=0; i<4; i++){
    flip_layer_destroy(layer[i]);
  }
  
  text_layer_destroy(text_layer_date);
  text_layer_destroy(text_layer_dow);
    
  destroy_inveter_layer();
  destroy_battery_layer();
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
  
  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  bluetooth_connection_service_subscribe(display_bt_layer);
}

static void deinit(void) {
  app_message_deregister_callbacks();
  bluetooth_connection_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
