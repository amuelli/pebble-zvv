#include "windows/note.h"

#include <pebble.h>

#define PADDING 8
#define SECTION_GAP 6

static Window *s_window;
static StatusBarLayer *s_status_bar;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_time_layer;
static TextLayer *s_head_layer;
static TextLayer *s_body_layer;

static char s_time[13];
static char s_head[129];
static char s_body[201];

static TextLayer *make_text_layer(int16_t x, int16_t y, int16_t w,
                                  const char *text, GFont font) {
  TextLayer *tl = text_layer_create(GRect(x, y, w, 2000));
  text_layer_set_text(tl, text);
  text_layer_set_font(tl, font);
  text_layer_set_overflow_mode(tl, GTextOverflowModeWordWrap);
  text_layer_set_background_color(tl, GColorClear);
  text_layer_set_text_color(tl, GColorBlack);
  return tl;
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int16_t w = bounds.size.w - 2 * PADDING;
  int16_t scroll_y = STATUS_BAR_LAYER_HEIGHT;

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  GRect scroll_bounds = GRect(0, scroll_y, bounds.size.w, bounds.size.h - scroll_y);
  s_scroll_layer = scroll_layer_create(scroll_bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));

  int16_t y = PADDING;

  if(s_time[0]) {
    s_time_layer = make_text_layer(PADDING, y, w, s_time,
                                   fonts_get_system_font(FONT_KEY_GOTHIC_14));
    GSize sz = text_layer_get_content_size(s_time_layer);
    text_layer_set_size(s_time_layer, GSize(w, sz.h));
    scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_time_layer));
    y += sz.h + SECTION_GAP;
  }

  s_head_layer = make_text_layer(PADDING, y, w, s_head,
                                 fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  GSize head_sz = text_layer_get_content_size(s_head_layer);
  text_layer_set_size(s_head_layer, GSize(w, head_sz.h));
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_head_layer));
  y += head_sz.h;

  if(s_body[0]) {
    y += SECTION_GAP + 2;
    s_body_layer = make_text_layer(PADDING, y, w, s_body,
                                   fonts_get_system_font(FONT_KEY_GOTHIC_14));
    GSize body_sz = text_layer_get_content_size(s_body_layer);
    text_layer_set_size(s_body_layer, GSize(w, body_sz.h));
    scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_body_layer));
    y += body_sz.h;
  }

  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, y + PADDING));
}

static void main_window_unload(Window *window) {
  status_bar_layer_destroy(s_status_bar);
  if(s_time_layer)  { text_layer_destroy(s_time_layer);  s_time_layer = NULL; }
  if(s_head_layer)  { text_layer_destroy(s_head_layer);  s_head_layer = NULL; }
  if(s_body_layer)  { text_layer_destroy(s_body_layer);  s_body_layer = NULL; }
  scroll_layer_destroy(s_scroll_layer);
}

void note_init() {
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = main_window_load,
    .unload = main_window_unload,
  });
}

void note_deinit() {
  window_destroy(s_window);
}

void note_show(const char *time_str, const char *head, const char *body) {
  strncpy(s_time, time_str ? time_str : "", sizeof(s_time) - 1);
  strncpy(s_head, head     ? head     : "", sizeof(s_head) - 1);
  strncpy(s_body, body     ? body     : "", sizeof(s_body) - 1);
  window_stack_push(s_window, true);
}
