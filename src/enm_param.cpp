#include "enm_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "cglobal.h"
#include "file.h"
#include "game.h"

// The order in which new weapons are added, by the most common way the
// player destroyed parts (0x45d964).
short g_weapon_order[3][7] = {
    {0, 2, 1, 4, 3, 6, 5},
    {0, 1, 3, 2, 4, 6, 5},
    {0, 3, 1, 6, 2, 4, 5},
};

// The weights of the tactics in the pick of RunBattle(); tactics from 9 on
// have the weight of tactic 9 (0x45d990).
char g_tactic_weights[12] = {1, 1, 1, 1, 1, 1, 3, 1, 3, 5, 0, 0};

// 0x419e60
CEnmParam::CEnmParam() {
  slots_ = NULL;
  records_ = NULL;
  name_ = "CEnmParam";
}

// 0x419f50
void CEnmParam::Finalize() {
  if (slots_ != NULL) {
    delete[] slots_;
    slots_ = NULL;
  }
  if (records_ != NULL) {
    delete[] records_;
    records_ = NULL;
  }
  plan_.Release();
}

// 0x419fa0
int CEnmParam::GetParams(short* out) {
  int count = state_.weapon_count + 6;
  const EnmSideStats* side = state_.sides;
  int total =
      plan_.kind_counts_[5] + plan_.kind_counts_[1] + plan_.kind_counts_[0];
  int side0 = side->Score() * 800 / total;
  ++side;
  out[0] = side->Score() * 800 / total;
  out[1] = state_.aim_speed;
  out[2] = static_cast<short>((state_.body_hit_points - 200) * 1.5f);
  out[3] = side0;
  out[4] = state_.fire_speed;
  out[5] = state_.toughness;
  for (int i = 6; i < count; ++i) {
    out[i] = state_.weapon_levels[state_.weapons[i - 6]];
  }
  return count;
}

// 0x41a080
int CEnmParam::GetResultSize() { return 360; }

// 0x41a090
void CEnmParam::FormatResult(char* text, int* columns, int* lines) {
  sprintf(text, "@@1STAGE %-3d                       @@0\n\n", g_global.stage);
  text += strlen(text);
  sprintf(text, "  BATTLE TIME         %3d.%02d SEC\n\n", battle_time_ / 60,
          battle_time_ % 60);
  text += strlen(text);
  int count = 2;
  if (g_game.rules().time_limit > 0 && g_game.rules().mode == 0) {
    sprintf(text, "  DEAD PENALTY  *%-2d   %3d.00 SEC\n\n", deaths_,
            deaths_ * 20);
    text += strlen(text);
    count = 3;
  }
  sprintf(text,
          "  BASE POINT        %8d PTS\n\n"
          "  RAPID DESTROY *%-2d %8d PTS\n\n"
          "  CHAIN DESTROY *%-2d %8d PTS\n\n"
          "@@1TOTAL ------------- %8d PTS\n\n",
          score_base_, rapid_count_, score_rapid_, chain_count_, score_chain_,
          score_base_ + score_rapid_ + score_chain_);
  *columns = 32;
  *lines = count + 4;
}

// 0x41a190
void CEnmParam::CountKill(EnmSlot* slot, int side) {
  slot->flags |= 2;
  const EnmPlanPart* part = slot->part;
  if (part->flags & 8) {
    int range = part->lerp_to - part->lerp_from;
    if (range != 0) {
      slot->low = slot->high =
          part->low +
          (part->high - part->low) * (slot->lerp_at - part->lerp_from) / range;
    } else {
      slot->low = slot->high = part->low;
    }
  } else {
    slot->low = part->low;
    slot->high = part->high;
  }
  if (part->flags & 0x100) {
    state_.flags |= EnmState::kArmor;
    slot->flags |= 8;
  }
  if (slot->part->flags & 0x20) {
    state_.flags |= EnmState::kTransform;
  }
  switch (slot->kind & 0xff0000) {
    case 0x10000:
      ++state_.sides[side].attack_kills;
      break;
    case 0x20000:
      ++state_.sides[side].wide_kills;
      break;
    case 0x40000:
      ++state_.sides[side].rapid_kills;
      break;
  }
  slot->hits = 0;
}

// 0x41a2a0
EnmSlot* CEnmParam::AllocSlot(unsigned int code, int kind) {
  if (slot_count_ >= slot_capacity_) {
    return NULL;
  }
  for (EnmSlot* slot = &slots_[slot_capacity_ - 1]; slot > slots_; --slot) {
    if (slot->code == 0) {
      slot->code = code;
      slot->part = plan_.Find(code, kind);
      slot->kind = slot->part->kind;
      slot->flags = 0;
      slot->hits = 0;
      slot->weapon = 0;
      slot->level = 0;
      slot->power = 10;
      slot->depth = 0;
      unsigned int ancestor = code & kCodeMask;
      if (ancestor != 0) {
        int depth = 0;
        do {
          ancestor = ParentCode(ancestor);
          ++depth;
        } while (ancestor != 0);
        slot->depth = depth;
      }
      if (slot->depth > state_.max_depth) {
        state_.max_depth = slot->depth;
      }
      if (slot->part->kind & 0x6000000) {
        slot->low = slot->part->lerp_from;
        slot->high = slot->part->lerp_to;
      } else {
        slot->low = slot->part->low;
        slot->high = slot->part->high;
      }
      ++slot_count_;
      return slot;
    }
  }
  return NULL;
}

// 0x41a3a0
void CEnmParam::ChooseWeapon(EnmSlot* slot, EnmSlot* parent) {
  unsigned int mask = slot->part->flags & state_.weapon_mask;
  if ((mask & (0x10000 << slot->weapon)) && (slot->flags & 4)) {
    slot->level = 0;
    return;
  }
  slot->flags &= ~4;
  int weapon;
  if (mask != 0) {
    if (slot->weapon < 0 && (parent->flags & 4)) {
      weapon = parent->weapon;
    } else {
      weapon = new_weapon_;
      if (weapon < 0) {
        weapon = state_.weapons[rand() % state_.weapon_count];
      }
    }
    while (!(mask & (0x10000 << weapon))) {
      weapon = rand() % 7;
    }
    slot->level = rand() % 3 + 1;
  } else {
    weapon = 0;
    slot->level = 1;
  }
  int max = state_.weapon_levels[weapon];
  int power = slot->power;
  if (power < max * 100 / 70) {
    power = max * 100 / 70;
  }
  if (power < parent->power) {
    power = parent->power;
  }
  if (power < 10) {
    power = 10;
  } else if (power > max) {
    power = max;
  }
  slot->weapon = weapon;
  slot->power = power;
}

// 0x41a4c0
void CEnmParam::LogDestroy(EnmSlot* slot, int chain, int rapid) {
  if (record_count_ < record_capacity_) {
    EnmDestroyRecord* record = &records_[record_count_];
    record->slot = slot;
    record->chain = chain;
    record->rapid = rapid;
    ++record_count_;
    if (rapid > 1) {
      EnmSideStats* side =
          &state_.sides[(slot->code & kSideMask) == 0x80000000];
      if (side->mode == 0) {
        ++side->rapid_kills;
      }
      AddClamped(&state_.toughness, rapid * 7);
      AddClamped(&state_.aim_speed, rapid * -5);
    }
    if (chain == 1) {
      AddClamped(&state_.toughness, rapid);
    }
  }
}

// 0x41a5b0
int CEnmParam::Save(File* file) {
  if (!file->WriteExact(&slot_count_, 4)) {
    return 0;
  }
  if (!file->WriteExact(&state_, sizeof(state_))) {
    return 0;
  }
  EnmSlot* slot = &slots_[slot_capacity_ - 1];
  for (int i = slot_count_; i > 0; --i, --slot) {
    if (!file->WriteExact(slot, 0x18)) {
      return 0;
    }
  }
  return 1;
}

// 0x41a660
int CEnmParam::Load(File* file) {
  if (!file->ReadExact(&slot_count_, 4)) {
    return 0;
  }
  if (!file->ReadExact(&state_, sizeof(state_))) {
    return 0;
  }
  EnmSlot* slot = &slots_[slot_capacity_ - 1];
  for (int i = slot_count_; i > 0; --i, --slot) {
    if (!file->ReadExact(slot, 0x18)) {
      return 0;
    }
    slot->part = plan_.Find(slot->code, slot->kind);
    if (slot->part == NULL) {
      return 0;
    }
    slot->lerp_at = 0;
  }
  return 1;
}

// 0x41a820
// The two early returns, each counting the part, give the original's
// registers for `this` (ebp), `code` (ecx), the slot flags (edx) and the
// totals (edi), and its shared ChooseWeapon() call; `short` option counts
// (the type of the fields) put `mode` in ecx and the counts in eax; `++count`
// after each call keeps the push of the kind first. The child codes are built
// in `code` itself: VC copies the later-defined operand of `|`, so with the
// parameter as the base the count is copied first (`mov ecx, edi; or ecx,
// ebx`), as in the original.
void CEnmParam::Grow(EnmSlot* parent, unsigned int code, int kind) {
  EnmSlot* slot = FindSlot(code);
  if (slot == NULL) {
    slot = AllocSlot(code, kind);
    if (slot != NULL) {
      if (!(slot->kind & 0x8000000)) {
        slot->flags = 1;
      }
      slot->weapon = -1;
      ChooseWeapon(slot, parent);
    }
    CountPart(slot);
    return;
  }
  if (!(slot->flags & 2)) {
    if (slot->level > 0 && --slot->level == 0) {
      ChooseWeapon(slot, parent);
    }
    CountPart(slot);
    return;
  }
  if ((state_.flags & EnmState::kArmor) && !(slot->flags & 8)) {
    int limit;
    if (slot->kind & 0x40000) {
      limit = 3;
    } else {
      limit = (slot->kind & 0x2000000) ? 5 : 4;
    }
    if (slot->hits > limit) {
      slot->flags |= 8;
    }
  }
  int count = slot->part->extra_child_count + slot->part->child_count;
  // The base code of the children: the side bits and the code shifted up.
  code = (code & kSideMask) | (code << 3);
  for (int i = 0; i < count; ++i) {
    Grow(slot, code | i, 0);
  }
  int mode = state_.sides[(code & kSideMask) == 0x80000000].mode;
  if ((mode & 1) && (slot->kind & 0x10000)) {
    short options = slot->part->option_count;
    for (int i = 0; i < options; ++i) {
      Grow(slot, code | count, 0x4010001);
      ++count;
    }
    slot->flags |= 0x1000;
  }
  if ((mode & 2) && (slot->kind & 0x20000)) {
    short options = slot->part->option_count;
    for (int i = 0; i < options; ++i) {
      Grow(slot, code | count, 0x4020001);
      ++count;
    }
    slot->flags |= 0x2000;
  }
  if (mode & 4) {
    count += slot->part->option_count;
    short options = slot->part->option4_count;
    for (int i = 0; i < options; ++i) {
      Grow(slot, code | count, 0x4040001);
      ++count;
    }
    slot->flags |= 0x4000;
  }
}

// 0x41aa60
// The slot's power is clamped in place, not with AddClamped(): the original
// loads slot->flags before adding to the power.
void CEnmParam::OnActorDestroyed(Actor* source, Actor* actor) {
  if (actor->id != 1) {
    return;
  }
  EnmSlot* slot = reinterpret_cast<EnmSlot*>(source->work[0]);
  if (slot != NULL) {
    slot->flags |= 4;
    slot->power += 50;
    if (slot->power > 0x400) {
      slot->power = 0x400;
    } else if (slot->power < 0) {
      slot->power = 0;
    }
    if (slot->power > g_enm_param.state_.weapon_levels[slot->weapon]) {
      g_enm_param.state_.weapon_levels[slot->weapon] = slot->power;
    }
  }
  AddClamped(&g_enm_param.state_.toughness, -50);
  ++g_global.entry.deaths;
  ++g_enm_param.deaths_;
}

// 0x41a730
// The loop's own return keeps the empty-log return out of line.
int CEnmParam::CountDestroyed(int count, unsigned int* kinds, int unique,
                              int all) {
  EnmDestroyRecord* record = records_;
  int i = record_count_;
  if (i > 0) {
    for (; i > 0; --i, ++record) {
      if ((record->slot->flags & 3) == 1 && (all || record->chain <= 2)) {
        int side;
        switch (record->slot->code & kSideMask) {
          case 0x40000000:
            side = 0;
            break;
          case 0x80000000:
            side = 1;
            break;
          default:
            continue;
        }
        if (unique) {
          if (kinds[side] == 0) {
            CountKill(record->slot, side);
            kinds[side] = record->slot->kind;
            --count;
            if (kinds[0] != 0 && kinds[1] != 0) {
              return count;
            }
          }
        } else {
          CountKill(record->slot, side);
          if (kinds[side] == 0) {
            kinds[side] = record->slot->kind;
          }
          --count;
        }
        if (count <= 0) {
          return count;
        }
      }
    }
    return count;
  }
  return count;
}

// 0x41ab20
int CEnmParam::Initialize(Application* app) {
  slot_capacity_ = 0x80;
  record_capacity_ = 0x101;
  slots_ = new EnmSlot[slot_capacity_];
  records_ = new EnmDestroyRecord[record_capacity_];
  if (slots_ == NULL || records_ == NULL) {
    Error("Out of memory.");
    return 0;
  }
  if (!plan_.Load("plan.dat")) {
    Error("Can't load Planning data.");
    return 0;
  }
  Actor::destroy_hook_ = OnActorDestroyed;
  return 1;
}

void CEnmParam::AddTactic(int id) {
  int count = state_.tactic_count;
  if (!(state_.tactic_mask & (1 << id))) {
    state_.tactics[count][0] = id;
    int cost = g_tactic_weights[id < 9 ? id : 9];
    if (count == 0) {
      state_.tactics[count][1] = cost;
    } else {
      state_.tactics[count][1] = state_.tactics[count - 1][1] + cost;
    }
    ++state_.tactic_count;
    state_.tactic_weight += cost;
    state_.tactic_mask |= 1 << id;
  }
}

// 0x41abc0
// `travel` is used twice (through __min), so VC computes it before `rate`
// and spills `rate` across the _ftol; `count` after `rate` keeps `rate` out
// of the dead `score` slot.
void CEnmParam::Plan(int score) {
  if (player_samples_ < 1) {
    player_samples_ = 1;
  }
  if (battle_time_ < 1) {
    battle_time_ = 1;
  }
  float travel = player_travel_ / player_samples_;
  float rate = static_cast<float>(aimed_frames_) / battle_time_;
  int count = static_cast<unsigned int>(score) / 300000 + 2;
  int add = __min(static_cast<int>(travel), 50);
  AddClamped(&state_.fire_speed, add);
  AddClamped(&state_.aim_speed, static_cast<int>(rate * 100.0f));
  state_.body_hit_points += state_.toughness * 70 / 1024 + 10;
  if (state_.body_hit_points > 1200) {
    state_.body_hit_points = 1200;
  }

  unsigned int kinds[2];
  kinds[0] = kinds[1] = 0;
  int left = CountDestroyed(count, kinds, 1, 0);
  new_weapon_ = -1;
  if (state_.weapon_count < 6 &&
      (g_global.stage <= 1 ||
       (left > 0 && g_global.stage >= state_.weapon_count * 2 - 2))) {
    int order;
    if (dir_frames_[0] > dir_frames_[1] && dir_frames_[0] > dir_frames_[2]) {
      order = 0;
    } else if (dir_frames_[1] > dir_frames_[0] &&
               dir_frames_[1] > dir_frames_[2]) {
      order = 1;
    } else if (dir_frames_[2] > dir_frames_[0] &&
               dir_frames_[2] > dir_frames_[1]) {
      order = 2;
    } else {
      order = rand() % 3;
    }
    for (int i = 0; i < 7; ++i) {
      int weapon = g_weapon_order[order][i];
      unsigned int bit = 0x10000 << weapon;
      if (!(state_.weapon_mask & bit)) {
        state_.weapon_mask |= bit;
        state_.weapons[state_.weapon_count] = weapon;
        ++state_.weapon_count;
        new_weapon_ = weapon;
        break;
      }
    }
    --left;
  }

  if (state_.sides[0].mode != 0 && state_.sides[1].mode != 0) {
    if (left > 2) {
      left = 2;
    }
    if (left > 0) {
      left = CountDestroyed(left, kinds, 0, 0);
      if (left > 0) {
        CountDestroyed(left, kinds, 1, 1);
      }
    }
  }
  EnmSideStats* side = state_.sides;
  for (int i = 0; i < 2; ++i, ++side) {
    if (side->mode == 0) {
      if (side->rapid_kills >= plan_.kind_counts_[2]) {
        side->mode = 4;
      } else {
        switch (kinds[i] & 0xff0000) {
          case 0x10000:
            if (side->attack_kills >= plan_.kind_counts_[0]) {
              side->mode = 1;
            } else if (side->wide_kills >= plan_.kind_counts_[1]) {
              side->mode = 2;
            }
            break;
          case 0x20000:
            if (side->wide_kills >= plan_.kind_counts_[1]) {
              side->mode = 2;
            } else if (side->attack_kills >= plan_.kind_counts_[0]) {
              side->mode = 1;
            }
            break;
        }
      }
      if (side->mode != 0) {
        // A new mode starts its own count.
        side->rapid_kills = 0;
      }
    }
  }
  if (state_.sides[0].progress >= 100 && state_.sides[1].progress >= 100) {
    for (int i = 0; i < 2; ++i) {
      int mode = state_.sides[i].mode;
      if (mode > 0 && mode <= 2) {
        state_.sides[i].mode = 3;
      }
    }
  }
  side = state_.sides;
  for (int i = 0; i < 2; ++i, ++side) {
    switch (side->mode) {
      case 1:
        side->progress = side->attack_kills * 100 /
                         (plan_.kind_counts_[0] + plan_.kind_counts_[3]);
        break;
      case 2:
        side->progress = side->wide_kills * 100 /
                         (plan_.kind_counts_[1] + plan_.kind_counts_[4]);
        break;
      case 3:
        side->progress =
            (side->rapid_kills + side->wide_kills + side->attack_kills) * 100 /
            (plan_.kind_counts_[5] + plan_.kind_counts_[1] +
             plan_.kind_counts_[0]);
        break;
      case 4:
        side->progress = (side->attack_kills + side->wide_kills) * 100 /
                         (plan_.kind_counts_[0] + plan_.kind_counts_[1] +
                          plan_.kind_counts_[3] + plan_.kind_counts_[4]);
        break;
      default:
        side->progress = 0;
        break;
    }
  }

  if (g_global.stage >= 1) {
    AddTactic(1);
  }
  if (state_.flags & EnmState::kArmor) {
    AddTactic(6);
  }
  // The modes of the sides that have grown to at least 65%.
  int side0_mode = state_.sides[0].mode;
  int side1_mode = state_.sides[1].mode;
  if (state_.sides[0].progress < 65) {
    side0_mode = 0;
  }
  if (state_.sides[1].progress < 65) {
    side1_mode = 0;
  }
  if ((state_.flags & EnmState::kTransform) &&
      (side0_mode != 0 || side1_mode != 0)) {
    AddTactic(8);
  }
  if (side0_mode & 1) {
    AddTactic(2);
    if (side1_mode & 1) {
      AddTactic(9);
    }
    if (side1_mode & 2) {
      AddTactic(10);
    }
    if (side1_mode & 4) {
      AddTactic(11);
    }
  }
  if (side0_mode & 2) {
    AddTactic(4);
    AddTactic(5);
    if (side1_mode & 1) {
      AddTactic(12);
    }
    if (side1_mode & 2) {
      AddTactic(13);
    }
    if (side1_mode & 4) {
      AddTactic(14);
    }
  }
  if (side0_mode & 4) {
    if (side1_mode & 1) {
      AddTactic(15);
    }
    if (side1_mode & 2) {
      AddTactic(16);
    }
    if (side1_mode & 4) {
      AddTactic(17);
    }
  }
  if (side1_mode & 1) {
    AddTactic(3);
  }
  if (side1_mode & 4) {
    AddTactic(7);
  }
  if (side1_mode & 2) {
    AddTactic(4);
    AddTactic(5);
  }

  for (int i = 0; i < 7; ++i) {
    weapon_parts_[i] = 0;
  }
  part_total_ = 0;
  power_total_ = 0;
  EnmSlot* root = FindSlot(0x40000000);
  Grow(root, 0x40000001, 0);
  Grow(root, 0x40000002, 0);
  root = FindSlot(0x80000000);
  Grow(root, 0x80000001, 0);
  Grow(root, 0x80000002, 0);
  ChooseTitle();
}

// 0x41b5b0
void CEnmParam::Reset() {
  memset(&state_, 0, sizeof(state_));
  state_.color = 0xff00ff00;
  state_.body_hit_points = 200;
  state_.max_depth = 0;
  slot_count_ = 0;
  for (int i = 0; i < slot_capacity_; ++i) {
    slots_[i].code = 0;
  }
  state_.tactic_mask = 0x40000;
  int count = state_.tactic_count;
  state_.tactics[count][0] = 0;
  int step = g_tactic_weights[0];
  if (count == 0) {
    state_.tactics[count][1] = step;
  } else {
    state_.tactics[count][1] = state_.tactics[count - 1][1] + step;
  }
  state_.tactic_weight += step;
  ++state_.tactic_count;
  state_.tactic_mask |= 1;
  AllocSlot(0xc0000000, 0);
  AllocSlot(0x40000000, 0);
  AllocSlot(0x80000000, 0);
  ClearTitles();
  Plan(0);
}
