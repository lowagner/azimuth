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

#include "azimuth/control/space.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "azimuth/constants.h"
#include "azimuth/control/paused.h"
#include "azimuth/control/util.h"
#include "azimuth/control/victory.h"
#include "azimuth/gui/audio.h"
#include "azimuth/gui/event.h"
#include "azimuth/gui/screen.h"
#include "azimuth/state/dialog.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/save.h"
#include "azimuth/state/space.h"
#include "azimuth/tick/script.h"
#include "azimuth/tick/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/space.h"

/*===========================================================================*/

static const char save_failed_paragraph[] =
  "$RERROR:$W Unable to save game.";
static const char save_success_paragraph[] =
  "Shields refilled and $Ggame saved$W.";

static az_space_state_t state;

static void position_ship_at_save_point_if_any(void) {
  const az_room_t *room = &state.planet->rooms[state.ship.player.current_room];
  state.ship.position = az_bounds_center(&room->camera_bounds);
  AZ_ARRAY_LOOP(node, state.nodes) {
    if (node->kind == AZ_NODE_CONSOLE &&
        node->subkind.console == AZ_CONS_SAVE) {
      state.ship.position = node->position;
      state.ship.angle = node->angle;
      break;
    }
  }
}

static void begin_saved_game(
    const az_planet_t *planet, const az_saved_games_t *saved_games,
    const az_preferences_t *prefs, int saved_game_index) {
  assert(saved_game_index >= 0);
  assert(saved_game_index < AZ_ARRAY_SIZE(saved_games->games));
  const az_saved_game_t *saved_game = &saved_games->games[saved_game_index];

  AZ_ZERO_OBJECT(&state);
  state.planet = planet;
  state.prefs = prefs;
  state.save_file_index = saved_game_index;
  state.mode = AZ_MODE_NORMAL;

  if (saved_game->present) {
    // Resume saved game:
    state.ship.player = saved_game->player;
    az_enter_room(&state, &planet->rooms[state.ship.player.current_room]);
    position_ship_at_save_point_if_any();
    az_after_entering_room(&state);
    state.console_help_message_cooldown = 10.0;
  } else {
    // Begin new game:
    az_init_player(&state.ship.player);
    state.intro = true;
    state.ship.player.current_room = planet->start_room;
    az_run_script(&state, planet->on_start);
  }
}

static bool save_current_game(az_saved_games_t *saved_games) {
  assert(state.save_file_index >= 0);
  assert(state.save_file_index < AZ_ARRAY_SIZE(saved_games->games));
  az_saved_game_t *saved_game = &saved_games->games[state.save_file_index];
  saved_game->present = true;
  saved_game->player = state.ship.player;
  return az_save_saved_games(saved_games);
}

static void update_held_controls(const az_key_id_t *key_for_control) {
  state.ship.controls.up_held =
    az_is_key_held(key_for_control[AZ_CONTROL_UP]);
  state.ship.controls.down_held =
    az_is_key_held(key_for_control[AZ_CONTROL_DOWN]);
  state.ship.controls.right_held =
    az_is_key_held(key_for_control[AZ_CONTROL_RIGHT]);
  state.ship.controls.left_held =
    az_is_key_held(key_for_control[AZ_CONTROL_LEFT]);
  state.ship.controls.fire_held =
    az_is_key_held(key_for_control[AZ_CONTROL_FIRE]);
  state.ship.controls.ordn_held =
    az_is_key_held(key_for_control[AZ_CONTROL_ORDN]);
  state.ship.controls.util_held =
    az_is_key_held(key_for_control[AZ_CONTROL_UTIL]);
}

az_space_action_t az_space_event_loop(
    const az_planet_t *planet, az_saved_games_t *saved_games,
    az_preferences_t *prefs, int saved_game_index) {
  begin_saved_game(planet, saved_games, prefs, saved_game_index);

  while (true) {
    // If we just finished the game intro, start us on the first room.
    if (state.intro && state.sync_vm.script == NULL) {
      state.intro = false;
      save_current_game(saved_games);
      az_enter_room(&state, &planet->rooms[planet->start_room]);
      position_ship_at_save_point_if_any();
      az_after_entering_room(&state);
    }

    // Tick the state and redraw the screen.
    update_held_controls(prefs->key_for_control);
    az_tick_space_state(&state, AZ_FRAME_TIME_SECONDS);
    az_tick_audio(&state.soundboard);
    az_start_screen_redraw(); {
      az_space_draw_screen(&state);
    } az_finish_screen_redraw();
    AZ_ZERO_OBJECT(&state.ship.controls);

    // Check the current mode; we may need to do something before we move on to
    // handling events.
    if (state.victory) {
      az_victory_event_loop(saved_games, &state.ship.player);
      return AZ_SA_VICTORY;
    } else if (state.mode == AZ_MODE_GAME_OVER) {
      // If we're at the end of the game over animation, exit this controller
      // and signal that we should transition to the game over screen
      // controller.
      if (state.game_over_mode.step == AZ_GOS_FADE_OUT &&
          state.game_over_mode.progress >= 1.0) {
        return AZ_SA_GAME_OVER;
      }
    } else if (state.mode == AZ_MODE_PAUSING) {
      // If we're at the end of the pausing fade-out, directly engage the
      // paused screen controller, and once it's done, either resume the game
      // or exit to the title screen, as appropriate.
      if (state.pausing_mode.step == AZ_PSS_FADE_OUT &&
          state.pausing_mode.fade_alpha == 1.0) {
        switch (az_paused_event_loop(planet, prefs, &state.ship)) {
          case AZ_PA_RESUME:
            state.pausing_mode.step = AZ_PSS_FADE_IN;
            break;
          case AZ_PA_EXIT_TO_TITLE:
            return AZ_SA_EXIT_TO_TITLE;
        }
      }
    } else if (state.mode == AZ_MODE_CONSOLE &&
               state.console_mode.step == AZ_CSS_SAVE) {
      // If we need to save the game, do so.
      const bool ok = save_current_game(saved_games);
      if (ok) az_set_message(&state, save_success_paragraph);
      else az_set_message(&state, save_failed_paragraph);
    }

    // Handle the event queue.
    az_event_t event;
    while (az_poll_event(&event)) {
      switch (event.kind) {
#ifndef NDEBUG
        // Cheat mode activated, for checking powerup descriptions.
        case AZ_EVENT_KEY_UP: {
          int maybe_upgrade = -1;
          if (!event.key.command) break;
          switch (event.key.id) {
            case AZ_KEY_G: {
              static az_upgrade_t gun = AZ_UPG_GUN_CHARGE;
              maybe_upgrade = gun;
              if (gun < AZ_UPG_GUN_BEAM) ++gun;
              break;
            }
            case AZ_KEY_U: {
              static az_upgrade_t special_upgrade = AZ_UPG_HYPER_ROCKETS;
              maybe_upgrade = special_upgrade;
              if (special_upgrade < AZ_UPG_MILLIWAVE_RADAR) ++special_upgrade;
              break;
            }
            case AZ_KEY_T:
              maybe_upgrade = AZ_UPG_TRACTOR_BEAM;
              break;
            case AZ_KEY_R: {
              static az_upgrade_t rocket_upgrade = AZ_UPG_ROCKET_AMMO_00;
              maybe_upgrade = rocket_upgrade;
              if (rocket_upgrade < AZ_UPG_ROCKET_AMMO_29) ++rocket_upgrade;
              break;
            }
            case AZ_KEY_B: {
              static az_upgrade_t bomb_upgrade = AZ_UPG_BOMB_AMMO_00;
              maybe_upgrade = bomb_upgrade;
              if (bomb_upgrade < AZ_UPG_BOMB_AMMO_19) ++bomb_upgrade;
              break;
            }
            case AZ_KEY_C: {
              static az_upgrade_t cap_upgrade = AZ_UPG_CAPACITOR_00;
              maybe_upgrade = cap_upgrade;
              if (cap_upgrade < AZ_UPG_CAPACITOR_11) ++cap_upgrade;
              break;
            }
            case AZ_KEY_S: {
              static az_upgrade_t shield_upgrade = AZ_UPG_SHIELD_BATTERY_00;
              maybe_upgrade = shield_upgrade;
              if (shield_upgrade < AZ_UPG_SHIELD_BATTERY_11) ++shield_upgrade;
              break;
            }
            default: break;
          }
          if (maybe_upgrade >= 0) {
            az_upgrade_t upgrade = maybe_upgrade;
            state.mode = AZ_MODE_UPGRADE;
            state.upgrade_mode = (az_upgrade_mode_data_t){
              .step = AZ_UGS_OPEN, .progress = 0.0, .upgrade = upgrade,
            };
            az_give_upgrade(&state.ship.player, upgrade);
          }
          break;
        }
#endif
        case AZ_EVENT_KEY_DOWN:
          if (state.skip.allowed && !state.skip.active) {
            assert(state.sync_vm.script != NULL);
            if (prefs->key_for_control[AZ_CONTROL_PAUSE] == event.key.id) {
              if (state.skip.cooldown < 1.0) {
                state.skip.cooldown = 4.0;
              } else {
                state.skip.active = true;
                state.skip.cooldown = 0.0;
              }
            } else if (event.key.id == AZ_KEY_RETURN) {
              state.skip.cooldown = (state.skip.cooldown > 0.0 ? 4.0 : 0.3);
            }
          }
          if (state.monologue.step != AZ_MLS_INACTIVE) {
            if (state.monologue.step == AZ_MLS_TALK) {
              state.monologue.step = AZ_MLS_WAIT;
              state.monologue.progress = 0.0;
              state.monologue.chars_to_print =
                state.monologue.paragraph_length;
            } else if (state.monologue.step == AZ_MLS_WAIT &&
                       event.key.id == AZ_KEY_RETURN) {
              assert(state.sync_vm.script != NULL);
              az_resume_script(&state, &state.sync_vm);
            }
            break;
          } else if (state.dialogue.step != AZ_DLS_INACTIVE) {
            if (state.dialogue.step == AZ_DLS_TALK) {
              state.dialogue.step = AZ_DLS_WAIT;
              state.dialogue.progress = 0.0;
              state.dialogue.chars_to_print = state.dialogue.paragraph_length;
            } else if (state.dialogue.step == AZ_DLS_WAIT &&
                       event.key.id == AZ_KEY_RETURN) {
              assert(state.sync_vm.script != NULL);
              az_resume_script(&state, &state.sync_vm);
            }
            break;
          } else if (state.mode == AZ_MODE_UPGRADE &&
                     !az_is_number_key(event.key.id)) {
            if (state.upgrade_mode.step == AZ_UGS_MESSAGE) {
              state.upgrade_mode.step = AZ_UGS_CLOSE;
              state.upgrade_mode.progress = 0.0;
            }
            break;
          } else if (state.mode == AZ_MODE_GAME_OVER) break;
          // Handle the keystroke:
          const az_control_id_t control_id =
            az_control_for_key(prefs, event.key.id);
          switch (control_id) {
            case AZ_CONTROL_CHARGE:
              az_select_gun(&state.ship.player, AZ_GUN_CHARGE);
              break;
            case AZ_CONTROL_FREEZE:
              az_select_gun(&state.ship.player, AZ_GUN_FREEZE);
              break;
            case AZ_CONTROL_TRIPLE:
              az_select_gun(&state.ship.player, AZ_GUN_TRIPLE);
              break;
            case AZ_CONTROL_HOMING:
              az_select_gun(&state.ship.player, AZ_GUN_HOMING);
              break;
            case AZ_CONTROL_PHASE:
              az_select_gun(&state.ship.player, AZ_GUN_PHASE);
              break;
            case AZ_CONTROL_BURST:
              az_select_gun(&state.ship.player, AZ_GUN_BURST);
              break;
            case AZ_CONTROL_PIERCE:
              az_select_gun(&state.ship.player, AZ_GUN_PIERCE);
              break;
            case AZ_CONTROL_BEAM:
              az_select_gun(&state.ship.player, AZ_GUN_BEAM);
              break;
            case AZ_CONTROL_ROCKETS:
              az_select_ordnance(&state.ship.player, AZ_ORDN_ROCKETS);
              break;
            case AZ_CONTROL_BOMBS:
              az_select_ordnance(&state.ship.player, AZ_ORDN_BOMBS);
              break;
            case AZ_CONTROL_PAUSE:
              if (state.mode == AZ_MODE_NORMAL &&
                  state.cutscene.scene == AZ_SCENE_NOTHING &&
                  !state.ship.autopilot.enabled) {
                state.mode = AZ_MODE_PAUSING;
                state.pausing_mode = (az_pausing_mode_data_t){
                  .step = AZ_PSS_FADE_OUT, .fade_alpha = 0.0
                };
              }
              break;
            case AZ_CONTROL_UP:
              state.ship.controls.up_pressed = true;
              break;
            case AZ_CONTROL_DOWN:
              state.ship.controls.down_pressed = true;
              break;
            case AZ_CONTROL_FIRE:
              state.ship.controls.fire_pressed = true;
              break;
            case AZ_CONTROL_UTIL:
              state.ship.controls.util_pressed = true;
              break;
            default:
              break;
          }
          break;
        default: break;
      }
    }
  }
}

/*===========================================================================*/
