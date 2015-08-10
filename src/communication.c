#include <pebble.h>
#include "communication.h"
#include "consts.h"
#include "departures.h"
#include "stations.h"

// Write message to buffer & send
void send_message(void){
  DictionaryIterator *iter;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Send message");

  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, KEY_CODE, 0x1);

  dict_write_end(iter);
  app_message_outbox_send();
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tCode, *tScope;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message"); 
  tCode = dict_find(iter, KEY_CODE);
  tScope = dict_find(iter, KEY_SCOPE);
  int code = (int)tCode->value->uint32;
  int scope = (int)tScope->value->uint32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Code: %d", code); 
  
  if(code == CODE_ARRAY_START) {
    int count = (int)dict_find(iter, KEY_COUNT)->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Items count: %d", count);
//       comm_array_size = count;
    if(scope == SCOPE_STA) {
      sta_set_count(count);
    } else if(scope == SCOPE_DEPS) {
      deps_set_count(count);
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Err!");
    }
//       snprintf(sb_printf_alloc(32), 32, "Loading...");
//       sb_printf_update(); 
  } else if(code == CODE_ARRAY_ITEM) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receiving item"); 
    int i = (int)dict_find(iter, KEY_ITEM)->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received item: %d", i); 
    if(scope == SCOPE_STA) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "station id: %d", (int)dict_find(iter, KEY_ID)->value->uint32);
      sta_set_item(i, (STA_Item){
        .id = dict_find(iter, KEY_ID)->value->uint32,
        .name = dict_find(iter, KEY_NAME)->value->cstring,
        .distance = dict_find(iter, KEY_DISTANCE)->value->uint32,
      });
    } else if(scope == SCOPE_DEPS) {
      deps_set_item(i, (DEP_Item){
        .id = dict_find(iter, KEY_ID)->value->uint32,
        .name = dict_find(iter, KEY_NAME)->value->cstring,
        .icon = dict_find(iter, KEY_ICON)->value->cstring,
        .color_fg = dict_find(iter, KEY_COLOR_FG)->value->uint32,
        .color_bg = dict_find(iter, KEY_COLOR_BG)->value->uint32,
        .direction = dict_find(iter, KEY_DIRECTION)->value->cstring,
        .time = dict_find(iter, KEY_DEP_TIME)->value->cstring,
        .delay = (int)dict_find(iter, KEY_DELAY)->value->uint32,
        .countdown = (int)dict_find(iter, KEY_COUNTDOWN)->value->uint32,
      });
    }
  }
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

void comm_get_deps(int stationId, int firstItemId) {
//  if(!comm_js_ready) {
//  comm_js_ready_cb = comm_query_tasks_cb;
//  comm_js_ready_cb_data = (void*)listId;
//  comm_is_available(); // show message if needed
//  return;
// }
// if(!comm_is_available())
// return;
// sb_show("Connecting...");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Querying departures for stationId=%d", stationId);
  DictionaryIterator *iter;
  Tuplet code = TupletInteger(KEY_CODE, CODE_GET);
  Tuplet scope = TupletInteger(KEY_SCOPE, SCOPE_DEPS);
  Tuplet tStationId = TupletInteger(KEY_ID, stationId);
  Tuplet tFirstItemId = TupletInteger(KEY_ITEM, firstItemId);

  app_message_outbox_begin(&iter);
  dict_write_tuplet(iter, &code);
  dict_write_tuplet(iter, &scope);
  dict_write_tuplet(iter, &tStationId);
  dict_write_tuplet(iter, &tFirstItemId);
  app_message_outbox_send();
}
  
void comm_init() {
  // Register AppMessage handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void comm_deinit() {
  app_message_deregister_callbacks();
}
