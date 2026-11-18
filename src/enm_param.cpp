#include "enm_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "cglobal.h"
#include "file.h"
#include "game.h"

// Steps added on a reset (0x45d990).
char g_reset_steps[12] = {1, 1, 1, 1, 1, 1, 3, 1, 3, 5, 0, 0};

// The code of the parent of the part at `code`.
static inline unsigned int ParentCode(unsigned int code) {
  return (code & CEnmParam::kSideMask) | ((code & CEnmParam::kCodeMask) >> 3);
}

// Adds `delta` to `value`, keeping it within 0..0x400.
static inline void AddClamped(short* value, int delta) {
  *value += delta;
  if (*value > 0x400) {
    *value = 0x400;
  } else if (*value < 0) {
    *value = 0;
  }
}

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
  int total =
      plan_.kind_counts_[5] + plan_.kind_counts_[1] + plan_.kind_counts_[0];
  const EnmSideStats* side = state_.sides;
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
      weapon = unknown_358_;
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
void CEnmParam::LogDestroy(EnmSlot* slot, int how, int chain) {
  if (record_count_ < record_capacity_) {
    EnmDestroyRecord* record = &records_[record_count_];
    record->slot = slot;
    record->how = how;
    record->chain = chain;
    ++record_count_;
    if (chain > 1) {
      EnmSideStats* side =
          &state_.sides[(slot->code & kSideMask) == 0x80000000];
      if (side->unknown_00_ == 0) {
        ++side->unknown_08_;
      }
      AddClamped(&state_.unknown_26_, chain * 7);
      AddClamped(&state_.unknown_2a_, chain * -5);
    }
    if (how == 1) {
      AddClamped(&state_.unknown_26_, chain);
    }
  }
}

// 0x41a5b0
// TODO: nonmatching (block order).
int CEnmParam::Save(File* file) {
  if (file->WriteExact(&slot_count_, 4) &&
      file->WriteExact(&state_, sizeof(state_))) {
    EnmSlot* slot = &slots_[slot_capacity_ - 1];
    for (int i = slot_count_; i > 0; --i, --slot) {
      if (!file->WriteExact(slot, 0x18)) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}

// 0x41a660
// TODO: nonmatching (block order).
int CEnmParam::Load(File* file) {
  if (file->ReadExact(&slot_count_, 4) &&
      file->ReadExact(&state_, sizeof(state_))) {
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
  return 0;
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
  ++g_global.deaths;
  ++g_enm_param.deaths_;
}

// 0x41a730
// TODO: nonmatching (the original returns early when nothing is logged).
int CEnmParam::CountDestroyed(int count, unsigned int* kinds, int unique,
                              int all) {
  EnmDestroyRecord* record = records_;
  for (int i = record_count_; i > 0; --i, ++record) {
    if ((record->slot->flags & 3) == 1 && (all || record->how <= 2)) {
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

// 0x41b5b0
// TODO: nonmatching (the zero register).
void CEnmParam::Reset() {
  memset(&state_, 0, sizeof(state_));
  state_.color = 0xff00ff00;
  state_.unknown_24_ = 200;
  state_.max_depth = 0;
  slot_count_ = 0;
  for (int i = 0; i < slot_capacity_; ++i) {
    slots_[i].code = 0;
  }
  state_.unknown_4c_ = 0x40000;
  int count = state_.step_count;
  state_.steps[count][0] = 0;
  char step = g_reset_steps[0];
  if (count == 0) {
    state_.steps[count][1] = step;
  } else {
    state_.steps[count][1] = state_.steps[count - 1][1] + step;
  }
  state_.step_total += step;
  ++state_.step_count;
  state_.unknown_4c_ |= 1;
  AllocSlot(0xc0000000, 0);
  AllocSlot(0x40000000, 0);
  AllocSlot(0x80000000, 0);
  ClearUnknown32c();
  Plan(0);
}

// 0x417c00
void CEnmParam::ClearUnknown32c() { unknown_32c_ = 0; }
