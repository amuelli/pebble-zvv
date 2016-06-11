#include <pebble.h>
#include "modules/communication.h"
#include "windows/departures.h"
#include "windows/stations.h"

// Write message to buffer & send
void send_message(void){
  DictionaryIterator *iter;
  /*APP_LOG(APP_LOG_LEVEL_DEBUG, "Send message");*/

  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, MESSAGE_KEY_code, 0x1);

  dict_write_end(iter);
  app_message_outbox_send();
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tCode, *tScope;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming message"); 
  tCode = dict_find(iter, MESSAGE_KEY_code);
  tScope = dict_find(iter, MESSAGE_KEY_scope);
  int code = (int)tCode->value->uint32;
  int scope = (int)tScope->value->uint32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Code: %d", code); 
  
  if(code == CODE_ARRAY_START) {
    int count = (int)dict_find(iter, MESSAGE_KEY_count)->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Items count: %d", count);
    if(scope == SCOPE_STA) {
      sta_set_count(count);
    } else if(scope == SCOPE_FAV) {
      sta_fav_set_count(count);
    } else if(scope == SCOPE_DEPS) {
      deps_set_count(count);
    } else {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Err!");
    }
  } else if(code == CODE_ARRAY_ITEM) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Receiving item"); 
    int i = (int)dict_find(iter, MESSAGE_KEY_item)->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received item: %d", i); 
    if(scope == SCOPE_STA) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "station id: %d", (int)dict_find(iter, MESSAGE_KEY_id)->value->uint32);
      sta_set_item(i, (STA_Item){
        .id = dict_find(iter, MESSAGE_KEY_id)->value->uint32,
        .name = dict_find(iter, MESSAGE_KEY_name)->value->cstring,
        .distance = dict_find(iter, MESSAGE_KEY_distance)->value->uint32,
      });
    } else if(scope == SCOPE_FAV) {
      sta_fav_set_item(i, (STA_Item){
        .id = dict_find(iter, MESSAGE_KEY_id)->value->uint32,
        .name = dict_find(iter, MESSAGE_KEY_name)->value->cstring,
        .distance = 0,
      });
    } else if(scope == SCOPE_DEPS) {
      deps_set_item(i, (DEP_Item){
        .id = dict_find(iter, MESSAGE_KEY_id)->value->uint32,
        .name = dict_find(iter, MESSAGE_KEY_name)->value->cstring,
        .icon = dict_find(iter, MESSAGE_KEY_icon)->value->cstring,
        .color_fg = dict_find(iter, MESSAGE_KEY_colorFg)->value->uint32,
        .color_bg = dict_find(iter, MESSAGE_KEY_colorBg)->value->uint32,
        .direction = dict_find(iter, MESSAGE_KEY_direction)->value->cstring,
        .time = dict_find(iter, MESSAGE_KEY_time)->value->cstring,
        .delay = (int)dict_find(iter, MESSAGE_KEY_delay)->value->uint32,
        .countdown = (int)dict_find(iter, MESSAGE_KEY_countdown)->value->uint32,
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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Querying departures for stationId=%d", stationId);
  DictionaryIterator *iter;
  Tuplet code = TupletInteger(MESSAGE_KEY_code, CODE_GET);
  Tuplet scope = TupletInteger(MESSAGE_KEY_scope, SCOPE_DEPS);
  Tuplet tStationId = TupletInteger(MESSAGE_KEY_id, stationId);
  Tuplet tFirstItemId = TupletInteger(MESSAGE_KEY_item, firstItemId);

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
