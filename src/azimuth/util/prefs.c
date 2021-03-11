/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include "azimuth/util/prefs.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "azimuth/util/key.h"
#include "azimuth/util/misc.h"

/*===========================================================================*/

#define DEFAULT_FULLSCREEN true

void az_reset_prefs_to_defaults(az_preferences_t *prefs) {
  *prefs = (az_preferences_t){
    .music_volume = 0.8, .sound_volume = 0.8,
    .speedrun_timer = false, .fullscreen_on_startup = DEFAULT_FULLSCREEN,
    .enable_hints = false,
    .key_for_control = {
      [AZ_CONTROL_UP] = AZ_KEY_UP_ARROW,
      [AZ_CONTROL_DOWN] = AZ_KEY_DOWN_ARROW,
      [AZ_CONTROL_RIGHT] = AZ_KEY_RIGHT_ARROW,
      [AZ_CONTROL_LEFT] = AZ_KEY_LEFT_ARROW,
      [AZ_CONTROL_FIRE] = AZ_KEY_PAGE_DOWN,
      [AZ_CONTROL_ORDN] = AZ_KEY_RIGHT_CONTROL,
      [AZ_CONTROL_UTIL] = AZ_KEY_HOME,
      [AZ_CONTROL_PAUSE] = AZ_KEY_ESCAPE,
      [AZ_CONTROL_BOMBS] = AZ_KEY_PAGE_UP,
      [AZ_CONTROL_CHARGE] = AZ_KEY_1,
      [AZ_CONTROL_FREEZE] = AZ_KEY_2,
      [AZ_CONTROL_TRIPLE] = AZ_KEY_3,
      [AZ_CONTROL_HOMING] = AZ_KEY_4,
      [AZ_CONTROL_PHASE] = AZ_KEY_5,
      [AZ_CONTROL_BURST] = AZ_KEY_6,
      [AZ_CONTROL_PIERCE] = AZ_KEY_7,
      [AZ_CONTROL_BEAM] = AZ_KEY_8,
      [AZ_CONTROL_ROCKETS] = AZ_KEY_END,
    },
  };
}


bool az_load_prefs_from_path(const char *filepath,
                             az_preferences_t *prefs_out) {
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "r");
  if (file == NULL) return false;
  const bool ok = az_load_prefs_from_file(file, prefs_out);
  fclose(file);
  return ok;
}

static bool read_bool(FILE *file, bool *out) {
  int value;
  if (fscanf(file, "=%d ", &value) < 1) return false;
  *out = (bool)value;
  return true;
}

static bool read_key(FILE *file, az_key_id_t *key_for_control,
                     az_control_id_t control_id) {
  int value;
  if (fscanf(file, "=%d ", &value) < 1) return false;
  if (value <= 0 || value >= AZ_NUM_ALLOWED_KEYS) return false;
  const az_key_id_t key = (az_key_id_t)value;
  if (!az_is_valid_prefs_key(key, control_id)) return false;
  key_for_control[control_id] = key;
  return true;
}

static bool read_volume(FILE *file, float *out) {
  double value;
  if (fscanf(file, "=%lf ", &value) < 1) return false;
  *out = fmin(fmax(0.0, value), 1.0);
  return true;
}

bool az_load_prefs_from_file(FILE *file, az_preferences_t *prefs_out) {
  assert(file != NULL);
  assert(prefs_out != NULL);

  az_preferences_t prefs;
  az_reset_prefs_to_defaults(&prefs);
  az_key_id_t *key_for_control = prefs.key_for_control;

  // Parse the file.
  if (fscanf(file, "@F ") < 0) return false;
  while (!feof(file)) {
    char name[3] = {0};
    name[0] = fgetc(file);
    name[1] = fgetc(file);
    if (strcmp(name, "mv") == 0) {
      if (!read_volume(file, &prefs.music_volume)) return false;
    }
    if (strcmp(name, "sv") == 0) {
      if (!read_volume(file, &prefs.sound_volume)) return false;
    }
    if (strcmp(name, "st") == 0) {
      if (!read_bool(file, &prefs.speedrun_timer)) return false;
    }
    if (strcmp(name, "fs") == 0) {
      if (!read_bool(file, &prefs.fullscreen_on_startup)) return false;
    }
    if (strcmp(name, "eh") == 0) {
      if (!read_bool(file, &prefs.enable_hints)) return false;
    }
    if (strcmp(name, "uk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_UP)) return false;
    }
    if (strcmp(name, "dk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_DOWN)) return false;
    }
    if (strcmp(name, "rk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_RIGHT)) return false;
    }
    if (strcmp(name, "lk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_LEFT)) return false;
    }
    if (strcmp(name, "fk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_FIRE)) return false;
    }
    if (strcmp(name, "ok") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_ORDN)) return false;
    }
    if (strcmp(name, "tk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_UTIL)) return false;
    }
    if (strcmp(name, "pk") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_PAUSE)) return false;
    }
    if (strcmp(name, "0k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_BOMBS)) return false;
    }
    if (strcmp(name, "1k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_CHARGE)) return false;
    }
    if (strcmp(name, "2k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_FREEZE)) return false;
    }
    if (strcmp(name, "3k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_TRIPLE)) return false;
    }
    if (strcmp(name, "4k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_HOMING)) return false;
    }
    if (strcmp(name, "5k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_PHASE)) return false;
    }
    if (strcmp(name, "6k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_BURST)) return false;
    }
    if (strcmp(name, "7k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_PIERCE)) return false;
    }
    if (strcmp(name, "8k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_BEAM)) return false;
    }
    if (strcmp(name, "9k") == 0) {
      if (!read_key(file, key_for_control, AZ_CONTROL_ROCKETS)) return false;
    }
  }

  // Require all keys to be different.
  for (int i = AZ_FIRST_CONTROL + 1; i < AZ_NUM_CONTROLS; ++i) {
    for (int j = AZ_FIRST_CONTROL; j < i; ++j) {
      if (key_for_control[i] == key_for_control[j]) return false;
    }
  }

  *prefs_out = prefs;
  return true;
}

bool az_save_prefs_to_path(const az_preferences_t *prefs,
                           const char *filepath) {
  assert(filepath != NULL);
  FILE *file = fopen(filepath, "w");
  if (file == NULL) return false;
  const bool ok = az_save_prefs_to_file(prefs, file);
  fclose(file);
  return ok;
}

bool az_save_prefs_to_file(const az_preferences_t *prefs, FILE *file) {
  assert(prefs != NULL);
  assert(file != NULL);
  const az_key_id_t* key_for_control = prefs->key_for_control;
  return (fprintf(
      file, "@F mv=%.03f sv=%.03f st=%d fs=%d eh=%d\n"
      "   uk=%d dk=%d rk=%d lk=%d fk=%d ok=%d tk=%d pk=%d\n"
      "   0k=%d 1k=%d 2k=%d 3k=%d 4k=%d 5k=%d 6k=%d 7k=%d 8k=%d 9k=%d\n",
      (double)prefs->music_volume, (double)prefs->sound_volume,
      (prefs->speedrun_timer ? 1 : 0), (prefs->fullscreen_on_startup ? 1 : 0),
      (prefs->enable_hints ? 1 : 0),
      key_for_control[AZ_CONTROL_UP],
      key_for_control[AZ_CONTROL_DOWN],
      key_for_control[AZ_CONTROL_RIGHT],
      key_for_control[AZ_CONTROL_LEFT],
      key_for_control[AZ_CONTROL_FIRE],
      key_for_control[AZ_CONTROL_ORDN],
      key_for_control[AZ_CONTROL_UTIL],
      key_for_control[AZ_CONTROL_PAUSE],
      key_for_control[AZ_CONTROL_BOMBS],
      key_for_control[AZ_CONTROL_CHARGE],
      key_for_control[AZ_CONTROL_FREEZE],
      key_for_control[AZ_CONTROL_TRIPLE],
      key_for_control[AZ_CONTROL_HOMING],
      key_for_control[AZ_CONTROL_PHASE],
      key_for_control[AZ_CONTROL_BURST],
      key_for_control[AZ_CONTROL_PIERCE],
      key_for_control[AZ_CONTROL_BEAM],
      key_for_control[AZ_CONTROL_ROCKETS]) >= 0);
}

bool az_is_valid_prefs_key(az_key_id_t key_id, az_control_id_t control_id) {
  switch (key_id) {
    case AZ_KEY_UNKNOWN:
    case AZ_KEY_RETURN:
      return false;
    case AZ_KEY_0:
    case AZ_KEY_1:
    case AZ_KEY_2:
    case AZ_KEY_3:
    case AZ_KEY_4:
    case AZ_KEY_5:
    case AZ_KEY_6:
    case AZ_KEY_7:
    case AZ_KEY_8:
    case AZ_KEY_9:
      // Number keys are only valid for the specific advanced weapon slot:
      return key_id - AZ_KEY_0 == control_id - AZ_CONTROL_BOMBS;
    default:
      return true;
  }
}

bool az_show_extra_weapon_key(const az_preferences_t *prefs,
                              unsigned int slot) {
  return prefs->key_for_control[AZ_CONTROL_BOMBS + slot] != AZ_KEY_0 + slot;
}

az_control_id_t az_control_for_key(const az_preferences_t *prefs,
                                   az_key_id_t key_id) {
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    if (prefs->key_for_control[i] == key_id) return i;
  }
  if (key_id >= AZ_KEY_0 && key_id <= AZ_KEY_9) {
    return AZ_CONTROL_BOMBS + (key_id - AZ_KEY_0);
  }
  return AZ_CONTROL_NONE;
}

/*===========================================================================*/
