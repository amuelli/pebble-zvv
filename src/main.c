#include <modules/communication.h>
#include <pebble.h>
#include <windows/departure.h>
#include <windows/departures.h>
#include <windows/note.h>
#include <windows/stations.h>

void init(void) {
  comm_init();
  sta_init();
  load_favorites();
  deps_init();
  dep_init();
  note_init();
  sta_show();
}

void deinit(void) {
  save_favorites();
  comm_deinit();
  sta_deinit();
  deps_deinit();
  dep_deinit();
  note_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
