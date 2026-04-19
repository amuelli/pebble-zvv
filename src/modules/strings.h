#pragma once

typedef enum {
  STR_DISRUPTIONS,
  STR_FAVORITE_STATIONS,
  STR_NEARBY_STATIONS,
  STR_COUNT
} StringKey;

void strings_set_language(const char *lang_code);
const char *str(StringKey key);
