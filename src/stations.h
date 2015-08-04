#pragma once

typedef struct {
  int id;
	char* name;
  int distance;
} STA_Item;

void sta_init();
void sta_deinit();
void sta_show();
void sta_set_count(int);
void sta_set_item(int, STA_Item);