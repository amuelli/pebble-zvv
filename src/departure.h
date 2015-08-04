#pragma once

typedef struct {
	int id;
	char* name;
	char* type;
  int color_fg;
  int color_bg;
  char* to;
  char* time;
  int delay;
  int countdown;
} DEP_Item;

void dep_init();
void dep_deinit();
void dep_show(int);
void dep_set_count(int);
void dep_set_item(int, DEP_Item);
