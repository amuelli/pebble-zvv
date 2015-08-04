#include <pebble.h>
#include <communication.h>
#include <stations.h>
#include <departures.h>

void init(void) {
  comm_init();
  sta_init();
  deps_init();
  sta_show();
}

void deinit(void) {
  comm_deinit();
  sta_deinit();
  deps_deinit();
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}
