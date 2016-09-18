#include <pebble.h>
#include "windows/departure.h"

static Window *departure;
static StatusBarLayer *s_status_bar;
static Layer *s_canvas_layer;

static GFont s_icons;
static GFont s_helvetic_bold;

static DEP_Item dep_item;

static void canvas_update_proc(Layer *layer, GContext *ctx) {

  GRect bounds = layer_get_bounds(layer);
  // draw line icon with colors
  GRect rect_bounds = GRect(0, 0, bounds.size.w, bounds.size.w/4);
  GRect text_bounds = GRect(0, 0, PBL_IF_ROUND_ELSE(bounds.size.w, 40), bounds.size.w/4);
  GColor color_bg;
  // correct some coloring
  switch(dep_item.color_bg) {
    case 9090335 : color_bg = GColorSpringBud; break;
    case 12703135 : color_bg = GColorInchworm; break;
    default : color_bg = GColorFromHEX(dep_item.color_bg);
  }
  GColor color_fg = GColorFromHEX(dep_item.color_fg);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(color_bg, GColorWhite));
  graphics_fill_rect(ctx, rect_bounds, 0, GCornerNone);
  if(gcolor_equal(color_bg, GColorWhite))
    graphics_draw_line(ctx, GPoint(0, rect_bounds.size.h), GPoint(rect_bounds.size.w, rect_bounds.size.h));
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(color_fg, GColorBlack));
  char * name = dep_item.name;
  GFont font;
  if(strlen(name) <= 2) {
    font = s_helvetic_bold;
  } else if(strlen(name) == 3) {
    font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  } else {
    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  }

  graphics_draw_text(ctx,
      name,
      font,
      text_bounds,
      GTextOverflowModeFill,
      PBL_IF_ROUND_ELSE(GTextAlignmentCenter,GTextAlignmentRight),
      NULL);

  char * direction = dep_item.direction;
  text_bounds.origin.x += PBL_IF_ROUND_ELSE(0, 44);
  text_bounds.origin.y += PBL_IF_ROUND_ELSE(28, 14);
  text_bounds.size.w = bounds.size.w - text_bounds.origin.x;
  text_bounds.size.h -= PBL_IF_ROUND_ELSE(28, 14);

  graphics_draw_text(ctx,
      direction,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      text_bounds,
      GTextOverflowModeFill,
      PBL_IF_ROUND_ELSE(GTextAlignmentCenter,GTextAlignmentLeft),
      NULL);

  graphics_context_set_text_color(ctx, GColorBlack);
  GRect countdown_bounds = GRect(0, 40, bounds.size.w, bounds.size.h - 38);
  static char s_buff[16];
  if(dep_item.countdown > 60) {
    strncpy(s_buff, ">1h", 16);
  } else if(dep_item.countdown > 0) {
    snprintf(s_buff, sizeof(s_buff), "00:%d%d", dep_item.countdown / 10, dep_item.countdown % 10);
  }
  graphics_draw_text(ctx,
      s_buff,
      fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS),
      countdown_bounds,
      GTextOverflowModeFill,
      GTextAlignmentCenter,
      NULL);

  GRect icon_bounds = GRect(0, 90, bounds.size.w, bounds.size.h - 38);
  char* icon_number;
  if (strcmp(dep_item.icon, "bus") == 0) {
    icon_number = "1";
  } else if (strcmp(dep_item.icon, "tram") == 0) {
    icon_number = "2";
  } else if (strcmp(dep_item.icon, "train") == 0) {
    icon_number = "3";
  } else if (strcmp(dep_item.icon, "boat") == 0) {
    icon_number = "4";
  } else if (strcmp(dep_item.icon, "funicular") == 0) {
    icon_number = "5";
  } else if (strcmp(dep_item.icon, "cable_car") == 0) {
    icon_number = "6";
  } else {
    icon_number = "";
  }
  graphics_draw_text(ctx,
      icon_number,
      s_icons,
      icon_bounds,
      GTextOverflowModeFill,
      GTextAlignmentCenter,
      NULL);
}

static void main_window_load(Window *window) {
  // Load the custom font
  s_icons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ICONS_48));
  s_helvetic_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_BOLD_28));

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
  // Destroy Custom Font
  fonts_unload_custom_font(s_icons);
  fonts_unload_custom_font(s_helvetic_bold);
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
  window_set_background_color(departure, GColorWhite);
  window_stack_push(departure, true);
}

