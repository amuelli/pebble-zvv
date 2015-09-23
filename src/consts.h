#pragma once
  
// Key values for AppMessage Dictionary
enum {
  KEY_CODE = 0, // message code
  KEY_SCOPE = 1, // message scope (station or departure)
  KEY_COUNT = 2, //
  KEY_ITEM = 5, // item index
  KEY_ID = 6, // zvv journey/station id
  KEY_NAME = 7, //
  KEY_ICON = 8, //
  KEY_COLOR_FG = 9, //
  KEY_COLOR_BG = 10, //
  KEY_DIRECTION = 11, //
  KEY_DEP_TIME = 12, //
  KEY_DELAY = 13, //
  KEY_COUNTDOWN = 14, //
  KEY_DISTANCE = 15, //
};
// Message codes
enum {
  CODE_READY = 0, // JS side is ready (and have access token)
  CODE_GET = 10, // get some info
  CODE_UPDATE = 11, // change some info (e.g. mark task as done/undone)
  CODE_ARRAY_START = 20, // start array transfer; app must allocate memory (includes count)
  CODE_ARRAY_ITEM = 21, // array item
  CODE_ARRAY_END = 22, // end array transfer; transaction is finished
  CODE_ITEM_UPDATED = 23, // very similar to ARRAY_ITEM, but contains only changed fields
  CODE_ERROR = 50, // some error occured; description may be included
};
// Scope codes
enum {
  SCOPE_STA = 0,
  SCOPE_FAV = 1,
  SCOPE_DEPS = 2,
};
