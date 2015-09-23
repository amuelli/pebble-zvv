#include <pebble.h>
#include "stations.h"
#include "communication.h"
#include "departures.h"

static int STATIONS_WINDOW_CELL_HEIGHT = 36;
static int PADDING = 2;
  
static Window *stations;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;

static int nrStations = -1; // how many nearby stations were loaded
static int sta_max_count = -1; // how many nearby stations we are expecting (i.e. buffer size)
static int nrFavorites = 0; // how many favorite stations are configured
static int sta_fav_max_count = -1; // how many favorite stations we are expecting (i.e. buffer size)
static STA_Item *sta_items = NULL; // buffer for nearby stations
static STA_Item *sta_fav_items = NULL; // buffer for favorite stations

static uint16_t get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return 2;
}

static void draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
  switch (section_index) {
    case 0:
      menu_cell_basic_header_draw(ctx, cell_layer, "Favourite Stations");
      break;
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Nearby Stations");
      break;
  }
}

static int16_t get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      if (nrFavorites==0) return 0;
      else return 22;
      break;
    case 1:
    default:
      return 22;
      break;
  }
}

static uint16_t get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  switch (section_index) {
    case 0:
      return nrFavorites;
      break;
    case 1:
      if(nrStations < 0) // not initialized
        return 0; // statusbar must already contain "Connecting..." message
      /*else if(nrStations == 0) // no data*/
        /*return 1;*/
      else
        return nrStations;
      break;
    default:
      return 0;
      break;
  }
}

static void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *idx, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  GRect frame = GRect(
      PADDING,
      -PADDING,
      bounds.size.w,
      bounds.size.h/2 - 2*PADDING
      );
  switch(idx->section) {
    case 0:
      menu_cell_basic_draw(ctx, cell_layer, sta_fav_items[idx->row].name, NULL, NULL);
      /*graphics_draw_text(ctx,*/
          /*sta_fav_items[idx->row].name,*/
          /*fonts_get_system_font(FONT_KEY_GOTHIC_18),*/
          /*frame,*/
          /*GTextOverflowModeTrailingEllipsis,*/
          /*GTextAlignmentLeft,*/
          /*NULL*/
          /*);*/
      break;
    case 1:
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
      break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *callback_context) {
  return STATIONS_WINDOW_CELL_HEIGHT;
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *idx, void *context) {
  switch(idx->section) {
    case 0:
      deps_show(sta_fav_items[idx->row].id, sta_fav_items[idx->row].name);
      break;
    case 1:
      deps_show(sta_items[idx->row].id, sta_items[idx->row].name);
      break;
  }
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
    .get_num_sections = (MenuLayerGetNumberOfSectionsCallback)get_num_sections_callback,
    .get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback)get_num_rows_callback,
    .get_cell_height = (MenuLayerGetCellHeightCallback)get_cell_height_callback,
    .get_header_height = (MenuLayerGetHeaderHeightCallback)get_header_height_callback,
    .draw_row = (MenuLayerDrawRowCallback)draw_row_callback,
    .draw_header = (MenuLayerDrawHeaderCallback)draw_header_callback,
    .select_click = (MenuLayerSelectCallback)select_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  menu_layer_set_highlight_colors(s_menu_layer, GColorCeleste, GColorBlack);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  status_bar_layer_destroy(s_status_bar);
  menu_layer_destroy(s_menu_layer);
}

static void sta_free_items() {
  for(int i=0; i<nrStations; i++) {
    free(sta_items[i].name);
  }
  free(sta_items);
  sta_items = NULL;
}

static void sta_fav_free_items() {
  for(int i=0; i<nrFavorites; i++) {
    free(sta_fav_items[i].name);
  }
  free(sta_fav_items);
  sta_fav_items = NULL;
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
  nrStations = 0;
}

void sta_fav_set_count(int count) {
  if(sta_fav_items)
    sta_fav_free_items();
  sta_fav_items = malloc(sizeof(STA_Item)*count);
  sta_fav_max_count = count;
  nrFavorites = 0;
}

void sta_set_item(int i, STA_Item data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "New item %d", i);
  sta_items[i].id = data.id;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "id %d", data.id);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "name %s of size %d", data.name, strlen(data.name));
  sta_items[i].name = malloc(strlen(data.name)+5);
  strcpy(sta_items[i].name, data.name);
  sta_items[i].distance = data.distance;
  nrStations++;
  menu_layer_reload_data(s_menu_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current count is %d", nrStations);
}

void sta_fav_set_item(int i, STA_Item data) {
  sta_fav_items[i].id = data.id;
  sta_fav_items[i].name = malloc(strlen(data.name)+5);
  strcpy(sta_fav_items[i].name, data.name);
  nrFavorites++;
  menu_layer_reload_data(s_menu_layer);
}

void load_favorites() {
  nrFavorites = persist_read_int(STORAGE_NR_FAVORITES);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Nr of saved favorites: %i", nrFavorites);
  sta_fav_items = malloc(sizeof(STA_Item)*nrFavorites);
  for (int i=0; i<nrFavorites; i++) {
    sta_fav_items[i].name = malloc(32);
    persist_read_string(STORAGE_FAVORITE_NAME+i, sta_fav_items[i].name, 32);
    sta_fav_items[i].id = persist_read_int(STORAGE_FAVORITE_ID+i);
  }
}

void save_favorites() {
  persist_write_int(STORAGE_NR_FAVORITES, nrFavorites);
  for (int i=0; i<nrFavorites; i++) {
    persist_write_string(STORAGE_FAVORITE_NAME+i, sta_fav_items[i].name);
    persist_write_int(STORAGE_FAVORITE_ID+i, sta_fav_items[i].id);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"saved %i favorite(s)", nrFavorites);
}
