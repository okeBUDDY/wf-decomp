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
  turn_speed_ = state_.unknown_2a_ * 2 / 1024 + 1;
  unknown_88_[0] = state_.unknown_28_ * 50 / 1024 + 10;
  state_.flags &= 0xffff0000;
  unknown_88_[2] = 0;
  unknown_88_[1] = 0;
  turret_count_ = 0;
  unknown_98_ = state_.max_depth * 2;
  if (unknown_98_ < 5) {
    unknown_98_ = 5;
  }
  battle_time_ = 0;
  unknown_344_ = 0;
  deaths_ = 0;
  unknown_338_[2] = 0;
  unknown_338_[1] = 0;
  unknown_338_[0] = 0;
  score_chain_ = 0;
  score_rapid_ = 0;
  score_base_ = 0;
  rapid_count_ = 0;
  chain_count_ = 0;
  unknown_17c_ = 0;
  player_samples_ = 0;
  player_travel_ = 0.0f;
  player_pos_[0] = player->transform->m[2][0];
  player_pos_[1] = player->transform->m[2][1];
  for (int i = 0; i < 7; ++i) {
    core_power_[0][i] = core_power_[1][i] = 0;
  }
  list_9c_.op = 0;
  list_b0_.op = 0;
  lists_c4_[0].op = 0;
  lists_c4_[1].op = 0;
  list_218_.Start(0, 0);
  EnmSlot* core = g_enm_param.FindSlot(kSideMask);
  core_count_ = 0;
  parts_alive_[0] = parts_alive_[1] = 0;
  armored_parts_[0] = armored_parts_[1] = 0;
  unknown_104_[0] = unknown_104_[1] = 0;
  Actor* boss = g_actor_system.Create(-32, NULL, 320.0f, -480.0f, 4, 0, core);
  --parts_alive_[0];
  core_total_ = core_count_;
  unknown_f4_[0] = parts_alive_[0];
  unknown_f4_[1] = parts_alive_[1];
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
  ++unknown_338_[player_dir_];
  if (state_.flags & 1) {
    ++unknown_344_;
  }
  if (state_.flags & 0x100) {
    int over;
    switch (list_218_.op) {
      case 0:
        over = Tactic4161d0(boss);
        break;
      case 1:
        over = Tactic4162f0(boss);
        break;
      case 2:
        over = Tactic416560(boss);
        break;
      case 3:
        over = Tactic416630(boss);
        break;
      case 4:
        over = Tactic4167a0(boss);
        break;
      case 5:
        over = Tactic4168e0(boss);
        break;
      case 6:
        over = Tactic417430(boss);
        break;
      case 7:
        over = Tactic4169c0(boss);
        break;
      case 8:
        over = Tactic417520(boss);
        break;
      case 18:
        over = Tactic4170c0(boss);
        break;
      case 9:
        over = Tactic416ac0(boss);
        break;
      case 10:
        over = Tactic416cb0(boss);
        break;
      case 11:
        over = Tactic416e10(boss);
        break;
      case 12:
      case 13:
        over = 1;
        break;
      case 14:
        over = Tactic417010(boss);
        break;
      case 15:
        over = Tactic4175d0(boss);
        break;
      case 16:
        over = Tactic4176f0(boss);
        break;
      case 17:
        over = Tactic4177e0(boss);
        break;
      default:
        over = 0;
        break;
    }
    if (over) {
      int next;
      if (state_.step_count <= 1) {
        next = 0;
      } else if (IsSideBeaten(boss, 0)) {
        next = 1;
      } else {
        do {
          int pick = rand() % state_.step_total;
          for (int i = 0; i < state_.step_count; ++i) {
            next = state_.steps[i][0];
            if (pick < state_.steps[i][1]) {
              break;
            }
          }
        } while (next == list_218_.op);
      }
      list_218_.Start(next, 0);
    } else {
      --list_218_.time;
    }
  }
  if (unknown_88_[1] > 0) {
    unknown_88_[1] -= unknown_88_[0];
    if (unknown_88_[1] < 0) {
      unknown_88_[1] = 0;
    }
  }
  if (unknown_88_[2] > 0) {
    unknown_88_[2] -= unknown_88_[0];
    if (unknown_88_[2] < 0) {
      unknown_88_[2] = 0;
    }
  }
  state_.flags &= ~0xff;
}
