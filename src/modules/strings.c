#include "modules/strings.h"

#include <string.h>

typedef enum { LANG_EN, LANG_DE, LANG_FR, LANG_IT } Language;

static Language s_lang = LANG_EN;

static const char *s_strings[STR_COUNT][4] = {
  /* STR_DISRUPTIONS        en               de              fr                  it */
  { "Disruptions",    "Störungen",    "Perturbations",  "Perturbazioni"  },
  /* STR_FAVORITE_STATIONS */
  { "Favorites",      "Favoriten",    "Favoris",        "Preferiti"      },
  /* STR_NEARBY_STATIONS */
  { "Nearby",         "In der Nähe",  "À proximité",    "Nelle vicinanze" },
};

void strings_set_language(const char *code) {
  if(!code) return;
  if(strncmp(code, "de", 2) == 0)      s_lang = LANG_DE;
  else if(strncmp(code, "fr", 2) == 0) s_lang = LANG_FR;
  else if(strncmp(code, "it", 2) == 0) s_lang = LANG_IT;
  else                                  s_lang = LANG_EN;
}

const char *str(StringKey key) {
  if(key >= STR_COUNT) return "";
  return s_strings[key][s_lang];
}
