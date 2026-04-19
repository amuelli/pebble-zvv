#pragma once

typedef struct {
  int id;
  char *name;
  char *icon;
  int color_fg;
  int color_bg;
  char *direction;
  char *time;
  int delay;
  int dep_time;
} DEP_Item;

void deps_init();
void deps_deinit();
void deps_show(int, const char *);
void deps_set_count(int);
void deps_set_item(int, DEP_Item);
void deps_set_note_count(int);
void deps_set_note(int, const char *, const char *, const char *);
