// The battle of CEnmParam: starting it and running the tactics each frame.
// Apart from the tactics (enm_tactics.cpp), so that VC does not keep `this`
// in ecx across the calls to them.
#include <math.h>
#include <stdlib.h>

#include "actor.h"
#include "actor_system.h"
#include "enm_param.h"
#include "hl_3d.h"

// 0x417170
Actor* CEnmParam::StartBattle(Actor* player) {
  record_count_ = 0;
  EnmSlot* slot = slots_;
  for (int i = slot_count_; i > 0; --i, ++slot) {
    slot->flags &= 0xffff;
  }
  turn_speed_ = state_.aim_speed * 2 / 1024 + 1;
  fire_rate_ = state_.fire_speed * 50 / 1024 + 10;
  state_.flags &= 0xffff0000;
  fire_wait_[1] = 0;
  fire_wait_[0] = 0;
  turret_count_ = 0;
  turret_weight_ = state_.max_depth * 2;
  if (turret_weight_ < 5) {
    turret_weight_ = 5;
  }
  battle_time_ = 0;
  aimed_frames_ = 0;
  deaths_ = 0;
  dir_frames_[2] = 0;
  dir_frames_[1] = 0;
  dir_frames_[0] = 0;
  score_chain_ = 0;
  score_rapid_ = 0;
  score_base_ = 0;
  rapid_count_ = 0;
  chain_count_ = 0;
  unused_17c_ = 0;
  player_samples_ = 0;
  player_travel_ = 0.0f;
  player_pos_[0] = player->transform->m[2][0];
  player_pos_[1] = player->transform->m[2][1];
  for (int i = 0; i < 7; ++i) {
    core_power_[0][i] = core_power_[1][i] = 0;
  }
  move_program_.op = 0;
  turn_program_.op = 0;
  fire_programs_[0].op = 0;
  fire_programs_[1].op = 0;
  tactic_program_.Start(0, 0);
  EnmSlot* core = g_enm_param.FindSlot(kSideMask);
  core_count_ = 0;
  parts_alive_[0] = parts_alive_[1] = 0;
  armored_parts_[0] = armored_parts_[1] = 0;
  transform_parts_[0] = transform_parts_[1] = 0;
  Actor* boss = g_actor_system.Create(-32, NULL, 320.0f, -480.0f, 4, 0, core);
  --parts_alive_[0];
  core_total_ = core_count_;
  start_parts_[0] = parts_alive_[0];
  start_parts_[1] = parts_alive_[1];
  trans_count_ = 0;
  return boss;
}

// 0x4178f0
void CEnmParam::RunBattle(Actor* player, Actor* boss) {
  if (boss == NULL) {
    return;
  }
  float x;
  float y;
  if (player != NULL) {
    x = player->transform->m[2][0];
    y = player->transform->m[2][1];
    if ((battle_time_ & 0x1f) == 0) {
      ++player_samples_;
      float dx = x - player_pos_[0];
      float dy = y - player_pos_[1];
      player_pos_[0] = x;
      player_pos_[1] = y;
      player_travel_ += sqrtf(dx * dx + dy * dy);
    }
  } else {
    x = player_pos_[0];
    y = player_pos_[1];
  }
  int angle = (boss->AngleTo(x, y) - boss->angle) & 0x3ff;
  if (angle >= 0x1c0 && angle <= 0x240) {
    player_dir_ = 0;
  } else if (angle >= 0x100 && angle <= 0x300) {
    player_dir_ = 1;
  } else {
    player_dir_ = 2;
  }
  ++battle_time_;
  ++dir_frames_[player_dir_];
  if (state_.flags & EnmState::kAimed) {
    ++aimed_frames_;
  }
  if (state_.flags & EnmState::kBattle) {
    int over;
    switch (tactic_program_.op) {
      case 0:
        over = TacticRoam(boss);
        break;
      case 1:
        over = TacticDashFlip(boss);
        break;
      case 2:
        over = TacticCharge(boss);
        break;
      case 3:
        over = TacticRearSparks(boss);
        break;
      case 4:
        over = TacticStream(boss);
        break;
      case 5:
        over = TacticRapidFire(boss);
        break;
      case 6:
        over = TacticGlowArmor(boss);
        break;
      case 7:
        over = TacticSpinDash(boss);
        break;
      case 8:
        over = TacticTransform(boss);
        break;
      case 18:
        over = TacticLastStand(boss);
        break;
      case 9:
        over = TacticChargeRearSparks(boss);
        break;
      case 10:
        over = TacticChargeFlip(boss);
        break;
      case 11:
        over = TacticRepeatCharge(boss);
        break;
      case 12:
      case 13:
        over = 1;
        break;
      case 14:
        over = TacticSpin(boss);
        break;
      case 15:
        over = TacticSpinGlowSparks(boss);
        break;
      case 16:
        over = TacticGlowRapidFire(boss);
        break;
      case 17:
        over = TacticSpinRam(boss);
        break;
      default:
        over = 0;
        break;
    }
    if (over) {
      int next;
      if (state_.tactic_count <= 1) {
        next = 0;
      } else if (IsSideBeaten(boss, 0)) {
        next = 1;
      } else {
        do {
          int pick = rand() % state_.tactic_weight;
          for (int i = 0; i < state_.tactic_count; ++i) {
            next = state_.tactics[i][0];
            if (pick < state_.tactics[i][1]) {
              break;
            }
          }
        } while (next == tactic_program_.op);
      }
      tactic_program_.Start(next, 0);
    } else {
      --tactic_program_.time;
    }
  }
  if (fire_wait_[0] > 0) {
    fire_wait_[0] -= fire_rate_;
    if (fire_wait_[0] < 0) {
      fire_wait_[0] = 0;
    }
  }
  if (fire_wait_[1] > 0) {
    fire_wait_[1] -= fire_rate_;
    if (fire_wait_[1] < 0) {
      fire_wait_[1] = 0;
    }
  }
  state_.flags &= ~0xff;
}
