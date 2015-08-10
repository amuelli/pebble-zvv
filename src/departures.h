#pragma once

typedef struct {
  int id;
  char* name;
  char* icon;
  int color_fg;
  int color_bg;
  char* direction;
  char* time;
  int delay;
  int countdown;
} DEP_Item;

void deps_init();
void deps_deinit();
void deps_show(int);
void deps_set_count(int);
void deps_set_item(int, DEP_Item);
