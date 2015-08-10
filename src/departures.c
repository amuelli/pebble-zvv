#include <pebble.h>
#include "departures.h"
#include "communication.h"
  
static int DEPARTURES_WINDOW_CELL_HEIGHT = 36;
static int ICON_SIZE = 30;
static int PADDING = 2;
  
static Window *departures;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;

static GBitmap *s_icon_bus_bitmap;
static GBitmap *s_icon_tram_bitmap;
static GBitmap *s_icon_train_bitmap;
static GBitmap *s_icon_boat_bitmap;
static GBitmap *s_icon_cable_bitmap;

static int deps_count = -1; // how many items were loaded ATM
static int deps_max_count = -1; // how many items we are expecting (i.e. buffer size)
static DEP_Item *deps_items = NULL; // buffer for items
  
static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if(deps_count < 0) // not initialized
    return 0; // statusbar must already contain "Connecting..." message
  else if(deps_count == 0) // no data
    return 1;
  else
    return deps_count;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *idx, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  // draw direction
  GRect frame = GRect(
    ICON_SIZE + 2*PADDING, 
    -3, 
    bounds.size.w - 2*ICON_SIZE,
    bounds.size.h/2
  );
  graphics_draw_text(ctx,
    deps_items[idx->row].direction,
    fonts_get_system_font(FONT_KEY_GOTHIC_18),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
  // draw time of scheduled departure plus delay
  frame.origin.y += 16;
  graphics_draw_text(ctx,
    deps_items[idx->row].time,
    fonts_get_system_font(FONT_KEY_GOTHIC_18),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
  // draw time until real time departure
  frame.origin.x = bounds.size.w - ICON_SIZE - PADDING;  
  frame.origin.y = 0;
  frame.size.h = ICON_SIZE;
  frame.size.w = ICON_SIZE;
  static char s_buff[16];
  int countdown = deps_items[idx->row].countdown;
  if(countdown > 0) {
    snprintf(s_buff, sizeof(s_buff), "%d'", countdown);
    graphics_draw_text(ctx,
      s_buff,
      fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
      frame,
      GTextOverflowModeFill,
      GTextAlignmentRight,
      NULL
    );
  } else {
    frame.origin.x = bounds.size.w - 20 - PADDING;  
    frame.origin.y = 5;
    frame.size.h = 20;
    frame.size.w = 20;
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    if (strcmp((char*)deps_items[idx->row].icon, "bus") == 0) {
        graphics_draw_bitmap_in_rect(ctx, s_icon_bus_bitmap, frame); 
    } else if (strcmp(deps_items[idx->row].icon, "tram") == 0) {
        graphics_draw_bitmap_in_rect(ctx, s_icon_tram_bitmap, frame); 
    } else if (strcmp(deps_items[idx->row].icon, "train") == 0) {
        graphics_draw_bitmap_in_rect(ctx, s_icon_train_bitmap, frame); 
    } else if (strcmp(deps_items[idx->row].icon, "boat") == 0) {
        graphics_draw_bitmap_in_rect(ctx, s_icon_boat_bitmap, frame); 
    } else if (strcmp(deps_items[idx->row].icon, "cable") == 0) {
        graphics_draw_bitmap_in_rect(ctx, s_icon_cable_bitmap, frame); 
    } else {
        snprintf(s_buff, sizeof(s_buff), "0'");
    }
  }
  // draw line icon with colors
  frame.origin.x = PADDING;
  frame.origin.y = (bounds.size.h / 2) - (ICON_SIZE / 2);
  frame.size.h = ICON_SIZE;
  frame.size.w = ICON_SIZE;
  GColor color_bg = GColorFromHEX(deps_items[idx->row].color_bg);
  GColor color_fg = GColorFromHEX(deps_items[idx->row].color_fg);
  graphics_context_set_fill_color(ctx, color_bg);
  graphics_fill_rect(ctx, frame, 3, GCornersAll); 
  if(gcolor_equal(color_bg, GColorWhite))
    graphics_draw_round_rect(ctx, frame, 3);
  graphics_context_set_text_color(ctx, color_fg);
  char * name = deps_items[idx->row].name;
  GFont font;
  if(strlen(name) <= 2) {
    frame.origin.y -= 2;
    font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
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
     NULL
  );
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *callback_context) {
  return DEPARTURES_WINDOW_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected item %d", (int)idx->row);
}

static void main_window_load(Window *window) {
  // load bitmaps
  s_icon_bus_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_BUS);
  s_icon_tram_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_TRAM);
  s_icon_train_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_TRAIN);
  s_icon_boat_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_BOAT);
  s_icon_cable_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ICON_CABLE);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // Create MenuLayer
  s_menu_layer = menu_layer_create(GRect(0,STATUS_BAR_LAYER_HEIGHT,bounds.size.w,bounds.size.h));
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
    .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
    .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
    .select_click = (MenuLayerSelectCallback)select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_highlight_colors(s_menu_layer, GColorCeleste, GColorBlack);
//   menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorWhite);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  // Destroy Bitmaps
  gbitmap_destroy(s_icon_bus_bitmap);
  gbitmap_destroy(s_icon_tram_bitmap);
  gbitmap_destroy(s_icon_train_bitmap);
  gbitmap_destroy(s_icon_boat_bitmap);
  gbitmap_destroy(s_icon_cable_bitmap);
  // Destroy TextLayer
  status_bar_layer_destroy(s_status_bar);
  menu_layer_destroy(s_menu_layer);
}

static void deps_free_items() {
  for(int i=0; i<deps_count; i++) {
    free(deps_items[i].name);
    free(deps_items[i].time);
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

void deps_show(int station_id) {
  window_stack_push(departures, true);
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
