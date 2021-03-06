#include <pebble.h>
#include "windows/departures.h"
#include "windows/departure.h"
#include "modules/communication.h"

static int DEPARTURES_WINDOW_CELL_HEIGHT = 36;
static int DEPARTURES_WINDOW_HEADER_HEIGHT = 22;
static int ICON_SIZE = 30;
static int PADDING = 2;

static Window *departures;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;

static GFont s_icons;
static GFont s_helvetic_bold;

static int deps_count = -1; // how many items were loaded ATM
static int deps_max_count = -1; // how many items we are expecting (i.e. buffer size)
static DEP_Item *deps_items = NULL; // buffer for items
static char stationName[32]; //name of station

static uint16_t get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 1;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorLightGray, GColorBlack));
  int lowerY = bounds.origin.y + bounds.size.h - 1;
  graphics_draw_line(ctx, GPoint(0, bounds.origin.y), GPoint(bounds.size.w, bounds.origin.y));
  graphics_draw_line(ctx, GPoint(0, lowerY), GPoint(bounds.size.w, lowerY));
  graphics_draw_text(ctx,
      stationName,
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(0, -2, bounds.size.w, 18),
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL);
}

static int16_t get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return DEPARTURES_WINDOW_HEADER_HEIGHT;
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if(deps_count < 0) // not initialized
    return 0; // statusbar must already contain "Connecting..." message
  else
    return deps_count;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *idx, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);

#if defined(PBL_ROUND)
  // get info of pixel row in the middle of menu row
  GBitmap *fb = graphics_capture_frame_buffer(ctx);
  GPoint sc_coord = layer_convert_point_to_screen(cell_layer, GPoint(0, bounds.size.h/2));
  GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, sc_coord.y);
  graphics_release_frame_buffer(ctx, fb);
  // adapt bounds for round displays
  bounds.origin.x = info.min_x + PADDING;
  bounds.size.w = info.max_x - info.min_x - PADDING;
#endif

  GRect frame = GRect(
      bounds.origin.x + ICON_SIZE + 3*PADDING,
      0,
      bounds.size.w - 2*ICON_SIZE - PADDING,
      bounds.size.h/2
      );

  // draw direction
  // expand frame width if countdown on the right is small
  if(deps_items[idx->row].countdown > 0 && deps_items[idx->row].countdown < 10) {
    frame.size.w += 10;
  }
  graphics_draw_text(ctx,
      deps_items[idx->row].direction,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      frame,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft,
      NULL);

  // draw time of scheduled departure plus delay
  frame.origin.y += 12;
  graphics_draw_text(ctx,
      deps_items[idx->row].time,
      fonts_get_system_font(FONT_KEY_GOTHIC_18),
      frame,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft,
      NULL);

  // draw time until real time departure
  frame.origin.x = bounds.origin.x + bounds.size.w - ICON_SIZE - PADDING;
  frame.origin.y = 0;
  frame.size.w = ICON_SIZE;
  frame.size.h = ICON_SIZE;
  if(deps_items[idx->row].countdown == 0) {
    // draw icon if departure is imminent
    char* icon_number;
    if (strcmp(deps_items[idx->row].icon, "bus") == 0) {
      icon_number = "1";
    } else if (strcmp(deps_items[idx->row].icon, "tram") == 0) {
      icon_number = "2";
    } else if (strcmp(deps_items[idx->row].icon, "train") == 0) {
      icon_number = "3";
    } else if (strcmp(deps_items[idx->row].icon, "boat") == 0) {
      icon_number = "4";
    } else if (strcmp(deps_items[idx->row].icon, "funicular") == 0) {
      icon_number = "5";
    } else if (strcmp(deps_items[idx->row].icon, "cable_car") == 0) {
      icon_number = "6";
    } else {
      icon_number = "";
    }
    frame.origin.x = bounds.origin.x + bounds.size.w - ICON_SIZE;
    frame.origin.y = 0;
    frame.size.w = ICON_SIZE+2;
    frame.size.h = ICON_SIZE;
    graphics_draw_text(ctx,
        icon_number,
        s_icons,
        frame,
        GTextOverflowModeWordWrap,
        GTextAlignmentCenter,
        NULL);
  } else {
    static char s_buff[16];
    if(deps_items[idx->row].countdown > 60) {
      strncpy(s_buff, ">1h", 16);
    } else if(deps_items[idx->row].countdown > 0) {
      snprintf(s_buff, sizeof(s_buff), "%d'", deps_items[idx->row].countdown);
    }
    graphics_draw_text(ctx,
        s_buff,
        fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
        frame,
        GTextOverflowModeFill,
        GTextAlignmentRight,
        NULL);
  }

  // draw line icon with colors
  frame.origin.x = bounds.origin.x + PADDING;
  frame.origin.y = (bounds.size.h / 2) - (ICON_SIZE / 2);
  frame.size.w = ICON_SIZE;
  frame.size.h = ICON_SIZE;

  GColor color_bg;
  // correct some coloring
  switch(deps_items[idx->row].color_bg) {
    case 9090335 : color_bg = GColorSpringBud; break;
    case 12703135 : color_bg = GColorInchworm; break;
    default : color_bg = GColorFromHEX(deps_items[idx->row].color_bg);
  }
  GColor color_fg = GColorFromHEX(deps_items[idx->row].color_fg);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(color_bg, GColorClear));
  graphics_fill_rect(ctx, frame, 3, GCornersAll);
  if(!gcolor_equal(color_bg, GColorWhite) || menu_cell_layer_is_highlighted(cell_layer)) {
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorWhite, GColorClear));
  }
  graphics_draw_round_rect(ctx, frame, 3);
  graphics_context_set_text_color(ctx, COLOR_FALLBACK(color_fg, GColorBlack));
  char * name = deps_items[idx->row].name;
  GFont font;
  if(strlen(name) == 1) {
    frame.origin.x += 1;
    frame.origin.y += 3;
    /*font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);*/
    font = s_helvetic_bold;
  } else if(strlen(name) == 2) {
    // correct position if 2nd digit is "1"
    if (strstr(name+1, "1") != NULL) {
      frame.origin.x += 2;
    }
    frame.origin.y += 3;
    /*font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);*/
    font = s_helvetic_bold;
    /*if(strlen(name) == 1) { frame.origin.x += 1; }*/
  } else if(strlen(name) == 3){
    frame.origin.y += 3;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  } else {
    frame.origin.y += 6;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  }

  graphics_draw_text(ctx,
      name,
      font,
      frame,
      GTextOverflowModeFill,
      GTextAlignmentCenter,
      NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return DEPARTURES_WINDOW_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected item %d", (int)idx->row);
  dep_show(deps_items[idx->row]);
}

static void main_window_load(Window *window) {
  // Load the custom font
  s_icons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ICONS_32));
  s_helvetic_bold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_BOLD_20));

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeNone);
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);

  // Create MenuLayer
#if defined(PBL_RECT)
  s_menu_layer = menu_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w, bounds.size.h));
#elif defined(PBL_ROUND)
  s_menu_layer = menu_layer_create(bounds);
#endif
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)get_num_sections_callback,
      .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
      .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
      .get_header_height = (MenuLayerGetHeaderHeightCallback)get_header_height_callback,
      .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
      .draw_header = (MenuLayerDrawHeaderCallback)draw_header_callback,
      .select_click = (MenuLayerSelectCallback)select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_highlight_colors(s_menu_layer, COLOR_FALLBACK(GColorCobaltBlue, GColorBlack), GColorWhite);
  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));
}

static void main_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "unload departures window");
  // Destroy Custom Font
  fonts_unload_custom_font(s_icons);
  // Destroy TextLayer
  status_bar_layer_destroy(s_status_bar);
  menu_layer_destroy(s_menu_layer);
  // Reset number of departures
  deps_count = -1;
}

static void deps_free_items() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Free %d items", deps_count);
  for(int i = 0; i < deps_count; i++) {
    free(deps_items[i].name);
    free(deps_items[i].icon);
    free(deps_items[i].time);
    free(deps_items[i].direction);
  }
  free(deps_items);
  deps_items = NULL;
}

/* Public functions */

void deps_init() {
  departures = window_create();
  window_set_background_color(departures, GColorClear);
  window_set_window_handlers(departures, (WindowHandlers) {
      .load = main_window_load,
      .unload = main_window_unload
  });
}

void deps_deinit() {
  window_destroy(departures);
  deps_free_items();
}

void deps_show(int station_id, char* station_name) {
  strcpy(stationName,station_name);
  window_stack_push(departures, true);
  menu_layer_reload_data(s_menu_layer);
  comm_get_deps(station_id, 0);
}

void deps_set_count(int count) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting count: %d", count);
  if(deps_items)
    deps_free_items();
  deps_items = malloc(sizeof(DEP_Item)*count);
  deps_max_count = count;
  deps_count = 0;
}

void deps_set_item(int i, DEP_Item data) {
  //ignore incomming items from previous requests
  if(deps_count >= 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "New item %d", i);
    //   assert(deps_max_count > 0, "Trying to set item while not initialized!");
    //   assert(deps_max_count > i, "Unexpected item index: %d, max count is %d", i, deps_max_count);

    deps_items[i].id = data.id;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "id %d", data.id);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "name %s of size %d", data.name, strlen(data.name));
    deps_items[i].name = malloc(strlen(data.name)+5);
    strcpy(deps_items[i].name, data.name);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "icon %s of size %d", data.icon, strlen(data.icon));
    deps_items[i].icon = malloc(strlen(data.icon)+5);
    strcpy(deps_items[i].icon, data.icon);
    deps_items[i].color_fg = data.color_fg;
    deps_items[i].color_bg = data.color_bg;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "direction %s of size %d", data.direction, strlen(data.direction));
    deps_items[i].direction = malloc(strlen(data.direction)+1);
    strcpy(deps_items[i].direction, data.direction);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "time %s", data.time);
    deps_items[i].time = malloc(strlen(data.time)+1);
    strcpy(deps_items[i].time, data.time);
    deps_items[i].delay = data.delay;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "delay %d", data.delay);
    deps_items[i].countdown = data.countdown;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "countdown %d", data.countdown);
    deps_count++;
    menu_layer_reload_data(s_menu_layer);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Current count is %d", deps_count);
  }
}
