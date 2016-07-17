#include <pebble.h>
#include "windows/departure.h"

static Window *departure;
static StatusBarLayer *s_status_bar;
static Layer *s_canvas_layer;

static DEP_Item dep_item;


static void canvas_update_proc(Layer *layer, GContext *ctx) {

  GRect bounds = layer_get_bounds(layer);
  // draw line icon with colors
  GRect rect_bounds = GRect(0, 0, bounds.size.w, 35);
  GRect text_bounds = GRect(2, 0, 28, 35);
  GColor color_bg;
  // correct some coloring
  switch(dep_item.color_bg) {
    case 9090335 : color_bg = GColorSpringBud; break;
    case 12703135 : color_bg = GColorInchworm; break;
    default : color_bg = GColorFromHEX(dep_item.color_bg);
  }
  GColor color_fg = GColorFromHEX(dep_item.color_fg);
  graphics_context_set_fill_color(ctx, color_bg);
  graphics_fill_rect(ctx, rect_bounds, 0, GCornerNone);
  if(gcolor_equal(color_bg, GColorWhite))
    graphics_draw_round_rect(ctx, rect_bounds, 0);
  graphics_context_set_text_color(ctx, color_fg);
  char * name = dep_item.name;
  GFont font;
  if(strlen(name) <= 2) {
    text_bounds.origin.y -= 2;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  } else if(strlen(name) == 3){
    text_bounds.origin.y += 3;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  } else {
    text_bounds.origin.y += 6;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  }

  graphics_draw_text(ctx,
     name,
     font,
     text_bounds,
     GTextOverflowModeFill,
     GTextAlignmentCenter,
     NULL
  );

}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  s_canvas_layer = layer_create(GRect(0,STATUS_BAR_LAYER_HEIGHT,bounds.size.w,bounds.size.h));
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  status_bar_layer_destroy(s_status_bar);
}

/* Public functions */

void dep_init() {
  departure = window_create();
  window_set_background_color(departure, GColorWhite);
  window_set_window_handlers(departure, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
}

void dep_deinit() {
  window_destroy(departure);
  //dep_free_items();
}

void dep_show(DEP_Item data) {
  dep_item = data;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "got item with id %d", dep_item.id);
  GColor color_bg = GColorFromHEX(dep_item.color_bg);
  window_set_background_color(departure, GColorWhite);
  window_stack_push(departure, true);
}

