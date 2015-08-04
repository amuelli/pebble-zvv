#include <pebble.h>
#include "stations.h"
#include "communication.h"
#include "departures.h"

static int STATIONS_WINDOW_CELL_HEIGHT = 36;
static int ICON_SIZE = 30;
static int PADDING = 2;
  
static Window *stations;
static MenuLayer *s_menu_layer;
static TextLayer *s_text_layer;
static StatusBarLayer *s_status_bar;

static int sta_count = -1; // how many items were loaded ATM
static int sta_max_count = -1; // how many items we are expecting (i.e. buffer size)
static STA_Item *sta_items = NULL; // buffer for items
  
static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if(sta_count < 0) // not initialized
    return 0; // statusbar must already contain "Connecting..." message
  else if(sta_count == 0) // no data
    return 1;
  else
    return sta_count;
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *idx, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  GRect frame = GRect(
    PADDING, 
    -PADDING, 
    bounds.size.w,
    bounds.size.h/2 - 2*PADDING
  );
  graphics_draw_text(ctx,
    sta_items[idx->row].name,
    fonts_get_system_font(FONT_KEY_GOTHIC_18),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
  static char distance[10];
  snprintf(distance, sizeof(distance), "%dm", sta_items[idx->row].distance);
  frame.origin.y = bounds.size.h/2 - 2*PADDING;
  graphics_draw_text(ctx,
    distance,
    fonts_get_system_font(FONT_KEY_GOTHIC_18),
    frame,
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
//   menu_cell_basic_draw(ctx, cell_layer, NULL, text, NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *callback_context) {
  return STATIONS_WINDOW_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Selected item %d", (int)idx->row); 
  deps_show(sta_items[idx->row].id);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Status Bar
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_separator_mode(s_status_bar, StatusBarLayerSeparatorModeDotted);
  status_bar_layer_set_colors(s_status_bar, GColorClear, GColorWhite);
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
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  
  // Text Layer for 'Searching nearby stations'
  s_text_layer = text_layer_create(GRect(0,80,bounds.size.w,bounds.size.h));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_text_layer, "Searching for nearby stations");
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  status_bar_layer_destroy(s_status_bar);
  text_layer_destroy(s_text_layer);
  menu_layer_destroy(s_menu_layer);
}	

static void sta_free_items() {
	for(int i=0; i<sta_count; i++) {
		free(sta_items[i].name);
  }
	free(sta_items);
	sta_items = NULL;
}

/* Public functions */

void sta_init() {
  stations = window_create();
  window_set_background_color(stations, GColorBlack);
  window_set_window_handlers(stations, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
}

void sta_deinit() {
  window_destroy(stations);
}

void sta_show() {
//   com_get_nearby_stations();
  window_stack_push(stations, true);
}

void sta_set_count(int count) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting count: %d", count);
	if(sta_items)
		sta_free_items();
	sta_items = malloc(sizeof(STA_Item)*count);
	sta_max_count = count;
	sta_count = 0;
}

void sta_set_item(int i, STA_Item data) {
  if(!layer_get_hidden((Layer *)s_text_layer))
    layer_set_hidden((Layer *)s_text_layer, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "New item %d", i);
	sta_items[i].id = data.id;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "id %d", data.id);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "name %s of size %d", data.name, strlen(data.name));
	sta_items[i].name = malloc(strlen(data.name)+5);
	strcpy(sta_items[i].name, data.name);
	sta_items[i].distance = data.distance;
	sta_count++;
	menu_layer_reload_data(s_menu_layer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Current count is %d", sta_count);
}