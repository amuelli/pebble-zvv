#include "windows/departure.h"

#include <pebble.h>

static Window *departure;
static StatusBarLayer *s_status_bar;
static Layer *s_canvas_layer;

static GFont s_icons;
static GFont s_helvetic_bold;

static DEP_Item dep_item;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  // draw line icon with colors
  GRect rect_bounds = GRect(0, 0, bounds.size.w, bounds.size.w / 4);
  GRect text_bounds = GRect(PBL_IF_ROUND_ELSE(0, 4), 0, PBL_IF_ROUND_ELSE(bounds.size.w, 40), bounds.size.w / 4);
  GColor color_bg;
  // correct some coloring
  switch(dep_item.color_bg) {
    case 9090335:
      color_bg = GColorSpringBud;
      break;
    case 12703135:
      color_bg = GColorInchworm;
      break;
    default:
      color_bg = GColorFromHEX(dep_item.color_bg);
  }
  GColor color_fg = GColorFromHEX(dep_item.color_fg);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(color_bg, GColorWhite));
  graphics_fill_rect(ctx, rect_bounds, 0, GCornerNone);
  if(gcolor_equal(color_bg, GColorWhite))
    graphics_draw_line(ctx, GPoint(0, rect_bounds.size.h), GPoint(rect_bounds.size.w, rect_bounds.size.h));
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(color_fg, GColorBlack));
  char *name = dep_item.name;
  int header_h = rect_bounds.size.h;
  GFont font;
  int font_h; // approximate rendered line height for each font
  if(strlen(name) <= 2) {
    font = s_helvetic_bold;
    font_h = 36;
  } else if(strlen(name) == 3) {
    font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    font_h = 36;
  } else {
    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    font_h = 28;
  }
  GRect icon_rect = text_bounds;
  int y_off = (header_h - font_h) / 2;
  if(y_off > 0) {
    icon_rect.origin.y += y_off;
    icon_rect.size.h -= y_off;
  }

  graphics_draw_text(ctx,
                     name,
                     font,
                     icon_rect,
                     GTextOverflowModeFill,
                     GTextAlignmentCenter,
                     NULL);

  char *direction = dep_item.direction;
  text_bounds.origin.x += PBL_IF_ROUND_ELSE(0, 44);
  text_bounds.origin.y += PBL_IF_ROUND_ELSE(28, 14);
  text_bounds.size.w = bounds.size.w - text_bounds.origin.x;
  text_bounds.size.h -= PBL_IF_ROUND_ELSE(28, 14);

  graphics_draw_text(ctx,
                     direction,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     text_bounds,
                     GTextOverflowModeFill,
                     PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft),
                     NULL);

  // compute remaining seconds against watch clock
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int now_secs = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
  int remaining = dep_item.dep_time - now_secs;
  if(remaining < -43200) remaining += 86400; // midnight wrap
  bool after = remaining <= 0;
  int elapsed = -remaining; // seconds since departure when after==true
  if(remaining < 0) remaining = 0;

  bool blink_on = t->tm_sec % 2 == 0;
  bool show_icon = after && (!blink_on || elapsed > 30); // blink for first 30s, then steady

  static const int DW = 20;
  static const int CW = 10;
  static char s_a[4], s_b[4], s_c[4];
  GFont leco = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);

  int drawable_h = bounds.size.h - STATUS_BAR_LAYER_HEIGHT;
  int cy = (drawable_h - 40) / 2; // center 40px countdown on the full screen

  if(after) {
    // draw + sign manually (LECO has no + glyph)
    int e_mins = elapsed / 60;
    int e_secs = elapsed % 60;
    int m_w = (e_mins >= 10) ? 2 * DW : DW;
    int plus_w = 18;
    int total = plus_w + m_w + CW + 2 * DW;
    int x = (bounds.size.w - total) / 2;
    graphics_fill_rect(ctx, GRect(x + 4, cy + 20, 10, 4), 0, GCornerNone);  // horizontal bar
    graphics_fill_rect(ctx, GRect(x + 7, cy + 17, 4, 10), 0, GCornerNone);  // vertical bar
    x += plus_w;
    snprintf(s_a, sizeof(s_a), "%d", e_mins);
    snprintf(s_b, sizeof(s_b), "%02d", e_secs);
    graphics_draw_text(ctx, s_a, leco, GRect(x, cy, m_w, 40),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    x += m_w;
    graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 12, 5, 5), 2, GCornersAll);
    graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 24, 5, 5), 2, GCornersAll);
    x += CW;
    graphics_draw_text(ctx, s_b, leco, GRect(x, cy, 2 * DW, 40),
                       GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  } else {
    // countdown before departure
    int minus_w = 18;
    if(remaining >= 3600) {
      snprintf(s_a, sizeof(s_a), "%d",  remaining / 3600);
      snprintf(s_b, sizeof(s_b), "%02d", (remaining % 3600) / 60);
      snprintf(s_c, sizeof(s_c), "%02d", remaining % 60);
      int h_w = (remaining >= 36000) ? 2 * DW : DW;
      int total = minus_w + h_w + CW + 2 * DW + CW + 2 * DW;
      int x = (bounds.size.w - total) / 2;
      graphics_fill_rect(ctx, GRect(x + 2, cy + 20, 10, 4), 0, GCornerNone);  // minus bar
      x += minus_w;
      graphics_draw_text(ctx, s_a, leco, GRect(x, cy, h_w, 40),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      x += h_w;
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 12, 5, 5), 2, GCornersAll);
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 24, 5, 5), 2, GCornersAll);
      x += CW;
      graphics_draw_text(ctx, s_b, leco, GRect(x, cy, 2 * DW, 40),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      x += 2 * DW;
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 12, 5, 5), 2, GCornersAll);
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 24, 5, 5), 2, GCornersAll);
      x += CW;
      graphics_draw_text(ctx, s_c, leco, GRect(x, cy, 2 * DW, 40),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    } else {
      int mins = remaining / 60;
      int secs = remaining % 60;
      snprintf(s_a, sizeof(s_a), "%d",  mins);
      snprintf(s_b, sizeof(s_b), "%02d", secs);
      int m_w = (mins >= 10) ? 2 * DW : DW;
      int total = minus_w + m_w + CW + 2 * DW;
      int x = (bounds.size.w - total) / 2;
      graphics_fill_rect(ctx, GRect(x + 2, cy + 20, 10, 4), 0, GCornerNone);  // minus bar
      x += minus_w;
      graphics_draw_text(ctx, s_a, leco, GRect(x, cy, m_w, 40),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      x += m_w;
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 12, 5, 5), 2, GCornersAll);
      graphics_fill_rect(ctx, GRect(x + CW/2 - 2, cy + 24, 5, 5), 2, GCornersAll);
      x += CW;
      graphics_draw_text(ctx, s_b, leco, GRect(x, cy, 2 * DW, 40),
                         GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    }
  }

  if(show_icon) {
    int icon_y = cy + 40;
    GRect icon_bounds = GRect(0, icon_y, bounds.size.w, drawable_h - icon_y);
    char *icon_number;
    if(strcmp(dep_item.icon, "bus") == 0 ||
       strcmp(dep_item.icon, "nachtbus") == 0) {
      icon_number = "1";
    } else if(strcmp(dep_item.icon, "tram") == 0) {
      icon_number = "2";
    } else if(strcmp(dep_item.icon, "train") == 0 ||
              strcmp(dep_item.icon, "s-bahn") == 0 ||
              strcmp(dep_item.icon, "interregio") == 0) {
      icon_number = "3";
    } else if(strcmp(dep_item.icon, "boat") == 0 ||
              strcmp(dep_item.icon, "schiff") == 0) {
      icon_number = "4";
    } else if(strcmp(dep_item.icon, "funicular") == 0) {
      icon_number = "5";
    } else if(strcmp(dep_item.icon, "cable_car") == 0) {
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
}

static void main_window_appear(Window *window) {
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void main_window_disappear(Window *window) {
  tick_timer_service_unsubscribe();
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

  s_canvas_layer = layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h));
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
  layer_destroy(s_canvas_layer);
}

/* Public functions */

void dep_init() {
  departure = window_create();
  window_set_background_color(departure, GColorWhite);
  window_set_window_handlers(departure, (WindowHandlers){
                                            .load = main_window_load,
                                            .unload = main_window_unload,
                                            .appear = main_window_appear,
                                            .disappear = main_window_disappear});
}

void dep_deinit() {
  window_destroy(departure);
}

void dep_show(DEP_Item data) {
  dep_item = data;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "got item with id %d", dep_item.id);
  window_set_background_color(departure, GColorWhite);
  window_stack_push(departure, true);
}
