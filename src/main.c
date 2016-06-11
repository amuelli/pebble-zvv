#include <pebble.h>
#include <modules/communication.h>
#include <windows/stations.h>
#include <windows/departures.h>

void init(void) {
  comm_init();
  sta_init();
  load_favorites();
  deps_init();
  sta_show();
}

void deinit(void) {
  save_favorites();
  comm_deinit();
  sta_deinit();
  deps_deinit();
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}
