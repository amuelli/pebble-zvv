/*
 * This module handles most of the watch-phone communication.
 */

#pragma once

void send_message(void);

// void comm_query_tasklists();
// void comm_query_tasks(int);
// void comm_query_task_details(int, int);
// void comm_update_task_status(int, int, bool);

// typedef void(* CommJsReadyCallback)(void *data);
// // Do something when JS will be ready
// void comm_on_js_ready(CommJsReadyCallback*, void*);
// // Returns false if there is no bluetooth connection
// // or there is unsent message waiting (usually if JS was not loaded yet)
// // If not available, show message in statusbar
// bool comm_is_available();
// bool comm_is_available_silent(); // don't update statusbar


void comm_get_deps(int, int);
void comm_init();
void comm_deinit();

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
