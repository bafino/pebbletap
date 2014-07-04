#include "pebble.h"

#define BUFFER_SIZE 30
  
enum {
  LISTEN_TAP,
  DETECTED_TAP
};

static Window *window;

static GRect window_frame;
static TextLayer *text_layer;

static int event_count = 0;
static char buffer[BUFFER_SIZE];

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // Process tap on ACCEL_AXIS_X, ACCEL_AXIS_Y or ACCEL_AXIS_Z
  // Direction is 1 or -1
  event_count++;
  snprintf(buffer, BUFFER_SIZE, "Tap detected! Count: %d", event_count);
  vibes_short_pulse();
  text_layer_set_text(text_layer, buffer);
  //Let the phone know so that notifications can be dismissed
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  Tuplet value = TupletInteger(DETECTED_TAP, 1);
  dict_write_tuplet(iter, &value);
  app_message_outbox_send();
  //Stop listening for tap events. We have what we need.
  accel_tap_service_unsubscribe();
  window_stack_pop_all(true);
}


static void app_message_handler(DictionaryIterator *iter, void *context) {
  app_log(0, "taptest.c", 37, "AppMessage Handler was called, yo.");
  Tuple *t = dict_find(iter, LISTEN_TAP);
  if (t != NULL && t->value->uint8) {
    text_layer_set_text(text_layer, "Listening for tap events.");
    accel_tap_service_subscribe(&accel_tap_handler);
  } else {
    text_layer_set_text(text_layer, "NOT listening for tap events.");
    accel_tap_service_unsubscribe();
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = window_frame = layer_get_frame(window_layer);
  text_layer = text_layer_create((GRect){ .origin = { 0, 30 }, .size = frame.size });
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  return;
}

static void init(void) {
  app_log(0, "taptest.c", 49, "taptest initialised.");
  app_message_register_inbox_received(&app_message_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, true /* Animated */);
  //window_set_background_color(window, GColorBlack);
}

static void deinit(void) {
  accel_tap_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
