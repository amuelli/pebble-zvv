#pragma once

// Storage codes
#define STORAGE_NR_FAVORITES     99
#define STORAGE_FAVORITE_NAME   100
#define STORAGE_FAVORITE_ID     200

typedef struct {
  int id;
  char* name;
  int distance;
} STA_Item;

void sta_init();
void sta_deinit();
void sta_show();
void sta_set_count(int);
void sta_fav_set_count(int);
void sta_set_item(int, STA_Item);
void sta_fav_set_item(int, STA_Item);
void save_favorites();
void load_favorites();
