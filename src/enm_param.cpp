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

// The length of each step; steps from 9 on have the length of step 9
// (0x45d990).
char g_step_costs[12] = {1, 1, 1, 1, 1, 1, 3, 1, 3, 5, 0, 0};

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
// TODO: nonmatching (register choice).
int CEnmParam::GetParams(short* out) {
  int count = state_.weapon_count + 6;
  const EnmSideStats* side = state_.sides;
  int total =
      plan_.kind_counts_[5] + plan_.kind_counts_[1] + plan_.kind_counts_[0];
  int side0 = side->Score() * 800 / total;
  ++side;
  out[0] = side->Score() * 800 / total;
  out[1] = state_.unknown_2a_;
  out[2] = static_cast<short>((state_.unknown_24_ - 200) * 1.5f);
  out[3] = side0;
  out[4] = state_.unknown_28_;
  out[5] = state_.unknown_26_;
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
void CEnmParam::Count(EnmSlot* slot, int side) {
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
    state_.flags |= 0x10000;
    slot->flags |= 8;
  }
  if (slot->part->flags & 0x20) {
    state_.flags |= 0x20000;
  }
  switch (slot->kind & 0xff0000) {
    case 0x10000:
      ++state_.sides[side].unknown_02_;
      break;
    case 0x20000:
      ++state_.sides[side].unknown_04_;
      break;
    case 0x40000:
      ++state_.sides[side].unknown_08_;
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
      unsigned int c = code & kCodeMask;
      if (c != 0) {
        int depth = 0;
        do {
          c = ParentCode(c);
          ++depth;
        } while (c != 0);
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
        ++side->unknown_08_;
      }
      AddClamped(&state_.unknown_26_, rapid * 7);
      AddClamped(&state_.unknown_2a_, rapid * -5);
    }
    if (chain == 1) {
      AddClamped(&state_.unknown_26_, rapid);
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
// TODO: nonmatching (register choice: the original keeps `this` in ebp and
// `code` in ecx).
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
  } else if (!(slot->flags & 2)) {
    if (slot->level > 0 && --slot->level == 0) {
      ChooseWeapon(slot, parent);
    }
  } else {
    if ((state_.flags & 0x10000) && !(slot->flags & 8)) {
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
    unsigned int base = (code & kSideMask) | (code << 3);
    int count = slot->part->extra_child_count + slot->part->child_count;
    for (int i = 0; i < count; ++i) {
      Grow(slot, base | i, 0);
    }
    int index = count;
    int mode = state_.sides[(base & kSideMask) == 0x80000000].mode;
    if ((mode & 1) && (slot->kind & 0x10000)) {
      int n = slot->part->option_count;
      for (int i = 0; i < n; ++i) {
        Grow(slot, base | index++, 0x4010001);
      }
      slot->flags |= 0x1000;
    }
    if ((mode & 2) && (slot->kind & 0x20000)) {
      int n = slot->part->option_count;
      for (int i = 0; i < n; ++i) {
        Grow(slot, base | index++, 0x4020001);
      }
      slot->flags |= 0x2000;
    }
    if (mode & 4) {
      index += slot->part->option_count;
      int n = slot->part->option4_count;
      for (int i = 0; i < n; ++i) {
        Grow(slot, base | index++, 0x4040001);
      }
      slot->flags |= 0x4000;
    }
    return;
  }
  ++weapon_parts_[slot->weapon];
  ++part_total_;
  power_total_ += slot->power;
}

// 0x41aa60
// TODO: nonmatching (the original loads slot->flags before adding to power).
void CEnmParam::OnActorDestroyed(Actor* source, Actor* actor) {
  if (actor->id != 1) {
    return;
  }
  EnmSlot* slot = reinterpret_cast<EnmSlot*>(source->work[0]);
  if (slot != NULL) {
    slot->flags |= 4;
    AddClamped(&slot->power, 50);
    if (slot->power > g_enm_param.state_.weapon_levels[slot->weapon]) {
      g_enm_param.state_.weapon_levels[slot->weapon] = slot->power;
    }
  }
  AddClamped(&g_enm_param.state_.unknown_26_, -50);
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
            Count(record->slot, side);
            kinds[side] = record->slot->kind;
            --count;
            if (kinds[0] != 0 && kinds[1] != 0) {
              return count;
            }
          }
        } else {
          Count(record->slot, side);
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

void CEnmParam::AddStep(int id) {
  int count = state_.step_count;
  if (!(state_.step_mask & (1 << id))) {
    state_.steps[count][0] = id;
    int cost = g_step_costs[id < 9 ? id : 9];
    if (count == 0) {
      state_.steps[count][1] = cost;
    } else {
      state_.steps[count][1] = state_.steps[count - 1][1] + cost;
    }
    ++state_.step_count;
    state_.step_total += cost;
    state_.step_mask |= 1 << id;
  }
}

// 0x41abc0
// TODO: nonmatching (close: the registers of the constants at the start and
// the pointer of the last loop over the sides).
void CEnmParam::Plan(int score) {
  if (player_samples_ < 1) {
    player_samples_ = 1;
  }
  if (battle_time_ < 1) {
    battle_time_ = 1;
  }
  int count = static_cast<unsigned int>(score) / 300000 + 2;
  int add = static_cast<int>(player_travel_ / player_samples_);
  float rate = static_cast<float>(unknown_344_) / battle_time_;
  if (add >= 50) {
    add = 50;
  }
  AddClamped(&state_.unknown_28_, add);
  AddClamped(&state_.unknown_2a_, static_cast<int>(rate * 100.0f));
  state_.unknown_24_ += state_.unknown_26_ * 70 / 1024 + 10;
  if (state_.unknown_24_ > 1200) {
    state_.unknown_24_ = 1200;
  }

  unsigned int kinds[2] = {0, 0};
  int left = CountDestroyed(count, kinds, 1, 0);
  new_weapon_ = -1;
  if (state_.weapon_count < 6 &&
      (g_global.stage <= 1 ||
       (left > 0 && g_global.stage >= state_.weapon_count * 2 - 2))) {
    int order;
    if (unknown_338_[0] > unknown_338_[1] &&
        unknown_338_[0] > unknown_338_[2]) {
      order = 0;
    } else if (unknown_338_[1] > unknown_338_[0] &&
               unknown_338_[1] > unknown_338_[2]) {
      order = 1;
    } else if (unknown_338_[2] > unknown_338_[0] &&
               unknown_338_[2] > unknown_338_[1]) {
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
  for (int i = 0; i < 2; ++i) {
    if (state_.sides[i].mode == 0) {
      if (state_.sides[i].unknown_08_ >= plan_.kind_counts_[2]) {
        state_.sides[i].mode = 4;
        state_.sides[i].unknown_08_ = 0;
      } else {
        switch (kinds[i] & 0xff0000) {
          case 0x10000:
            if (state_.sides[i].unknown_02_ >= plan_.kind_counts_[0]) {
              state_.sides[i].mode = 1;
              state_.sides[i].unknown_08_ = 0;
            } else if (state_.sides[i].unknown_04_ >= plan_.kind_counts_[1]) {
              state_.sides[i].mode = 2;
              state_.sides[i].unknown_08_ = 0;
            }
            break;
          case 0x20000:
            if (state_.sides[i].unknown_04_ >= plan_.kind_counts_[1]) {
              state_.sides[i].mode = 2;
              state_.sides[i].unknown_08_ = 0;
            } else if (state_.sides[i].unknown_02_ >= plan_.kind_counts_[0]) {
              state_.sides[i].mode = 1;
              state_.sides[i].unknown_08_ = 0;
            }
            break;
        }
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
  for (int i = 0; i < 2; ++i) {
    EnmSideStats* side = &state_.sides[i];
    switch (side->mode) {
      case 1:
        side->progress = side->unknown_02_ * 100 /
                         (plan_.kind_counts_[0] + plan_.kind_counts_[3]);
        break;
      case 2:
        side->progress = side->unknown_04_ * 100 /
                         (plan_.kind_counts_[1] + plan_.kind_counts_[4]);
        break;
      case 3:
        side->progress =
            (side->unknown_08_ + side->unknown_04_ + side->unknown_02_) * 100 /
            (plan_.kind_counts_[5] + plan_.kind_counts_[1] +
             plan_.kind_counts_[0]);
        break;
      case 4:
        side->progress = (side->unknown_02_ + side->unknown_04_) * 100 /
                         (plan_.kind_counts_[0] + plan_.kind_counts_[1] +
                          plan_.kind_counts_[3] + plan_.kind_counts_[4]);
        break;
      default:
        side->progress = 0;
        break;
    }
  }

  if (g_global.stage >= 1) {
    AddStep(1);
  }
  if (state_.flags & 0x10000) {
    AddStep(6);
  }
  int mode0 = state_.sides[0].mode;
  int mode1 = state_.sides[1].mode;
  if (state_.sides[0].progress < 65) {
    mode0 = 0;
  }
  if (state_.sides[1].progress < 65) {
    mode1 = 0;
  }
  if ((state_.flags & 0x20000) && (mode0 != 0 || mode1 != 0)) {
    AddStep(8);
  }
  if (mode0 & 1) {
    AddStep(2);
    if (mode1 & 1) {
      AddStep(9);
    }
    if (mode1 & 2) {
      AddStep(10);
    }
    if (mode1 & 4) {
      AddStep(11);
    }
  }
  if (mode0 & 2) {
    AddStep(4);
    AddStep(5);
    if (mode1 & 1) {
      AddStep(12);
    }
    if (mode1 & 2) {
      AddStep(13);
    }
    if (mode1 & 4) {
      AddStep(14);
    }
  }
  if (mode0 & 4) {
    if (mode1 & 1) {
      AddStep(15);
    }
    if (mode1 & 2) {
      AddStep(16);
    }
    if (mode1 & 4) {
      AddStep(17);
    }
  }
  if (mode1 & 1) {
    AddStep(3);
  }
  if (mode1 & 4) {
    AddStep(7);
  }
  if (mode1 & 2) {
    AddStep(4);
    AddStep(5);
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
  state_.unknown_24_ = 200;
  state_.max_depth = 0;
  slot_count_ = 0;
  for (int i = 0; i < slot_capacity_; ++i) {
    slots_[i].code = 0;
  }
  state_.step_mask = 0x40000;
  int count = state_.step_count;
  state_.steps[count][0] = 0;
  int step = g_step_costs[0];
  if (count == 0) {
    state_.steps[count][1] = step;
  } else {
    state_.steps[count][1] = state_.steps[count - 1][1] + step;
  }
  state_.step_total += step;
  ++state_.step_count;
  state_.step_mask |= 1;
  AllocSlot(0xc0000000, 0);
  AllocSlot(0x40000000, 0);
  AllocSlot(0x80000000, 0);
  ClearTitles();
  Plan(0);
}
