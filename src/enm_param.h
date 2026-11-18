// CEnmParam: the enemy parameters (g_enm_param, a System named
// "CEnmParam"). It grows the boss from stage to stage. Every part the
// player destroys is logged with how it was destroyed, and the next boss
// is planned from that log. EnmPlan holds the part templates loaded from
// plan.dat.
#ifndef WF_ENM_PARAM_H_
#define WF_ENM_PARAM_H_

#include "system.h"

class Actor;
class File;

// A weapon or extra of plan.dat (0x1c bytes); the start of a part
// template.
struct EnmPlanItem {
  unsigned int code;   // 0x00
  unsigned int flags;  // 0x04
  unsigned int kind;   // 0x08
  short x;             // 0x0c, relative to the parent
  short y;             // 0x0e
  short unknown_10_;   // 0x10
  short low;           // 0x12
  short high;          // 0x14
  short debris;        // 0x16, an index into g_debris_controls
  short pat_file;      // 0x18, see PlanPatFile()
  short pat;           // 0x1a
};

// A part template of plan.dat (0x48 bytes).
struct EnmPlanPart : EnmPlanItem {
  enum Flags {
    kMirrorX = 0x1,
    kMirrorY = 0x2,
    kGuard = 0x4,     // takes no damage
    kAimHigh = 0x40,  // starts at `high`
    kTurret = 0x80,   // its first child turns it
  };

  short unknown_1c_[6];     // 0x1c
  int score;                // 0x28, of a linked part
  short hit_points;         // 0x2c, of a linked part
  short x1;                 // 0x2e, where it is while armed (flags & 3 == 1)
  short y1;                 // 0x30
  short unknown_32_;        // 0x32
  short lerp_from;          // 0x34
  short lerp_to;            // 0x36
  short child_count;        // 0x38
  short option_count;       // 0x3a, children added in mode 1 or 2
  short option4_count;      // 0x3c, children added in mode 4
  short extra_child_count;  // 0x3e
  short item_count;         // 0x40
  short unknown_42_;        // 0x42
  EnmPlanItem* item;        // 0x44, an index into EnmPlan::items_ on disk
};

// The contents of plan.dat (vtable 0x4514ac, enm_plan.cpp).
class EnmPlan {
 public:
  enum { kWeaponKind = 0x8000001 };

  EnmPlan();  // 0x41b680
  // 0x401a60, inline copy 0x401a50
  virtual ~EnmPlan() { Release(); }
  void Release();  // 0x41b6a0
  // Loads `path`; returns 0 on failure.
  int Load(const char* path);  // 0x41b790
  // The template at `code` of `kind` (any kind for 0), NULL without one.
  const EnmPlanPart* Find(unsigned int code, int kind);  // 0x41b6f0

 private:
  friend class CEnmParam;

  // The header of plan.dat (0x58 bytes).
  int unknown_04_;        // 0x04
  int part_count_;        // 0x08
  int weapon_count_;      // 0x0c
  int item_count_;        // 0x10
  EnmPlanPart root_;      // 0x14
  EnmPlanPart* parts_;    // 0x5c
  EnmPlanItem* weapons_;  // 0x60
  EnmPlanItem* items_;    // 0x64
  // Parts counted by kind.
  int kind_counts_[6];  // 0x68
};

// A list (vtable 0x4514b0, 0x14 bytes).
// A movement program of the boss (CEnmParam::list_9c_...): the operation it
// runs and how far it got.
class EnmList {
 public:
  EnmList() {  // 0x419e40
    time = 0;
    step = 0;
    arg = 0;
    op = 0;
  }
  // 0x401a90, scalar deleting dtor 0x401aa0
  virtual ~EnmList() {}

  int arg;   // 0x04, of the operation
  int time;  // 0x08, frames since the operation started
  int op;    // 0x0c, the operation, 0: none
  int step;  // 0x10, within the operation
};

// A part of the boss being planned (0x20 bytes).
struct EnmSlot {
  unsigned int code;        // 0x00, 0 while the slot is free
  unsigned int kind;        // 0x04, of the template
  unsigned int flags;       // 0x08
  short depth;              // 0x0c, in the part tree
  short low;                // 0x0e
  short high;               // 0x10
  short hits;               // 0x12
  char weapon;              // 0x14
  char level;               // 0x15
  short power;              // 0x16
  const EnmPlanPart* part;  // 0x18
  short lerp_at;            // 0x1c, between part->lerp_from and lerp_to
};

// A logged destruction (8 bytes).
struct EnmDestroyRecord {
  EnmSlot* slot;  // 0x00
  short chain;    // 0x04, the parts destroyed with it (a power of 2)
  short rapid;    // 0x06, how fast it was destroyed (a power of 2)
};

// Adds `delta` to `value`, keeping it within 0..0x400.
inline void AddClamped(short* value, int delta) {
  *value += delta;
  if (*value > 0x400) {
    *value = 0x400;
  } else if (*value < 0) {
    *value = 0;
  }
}

// Destructions counted for one side of the boss (10 bytes).
struct EnmSideStats {
  int Score() const {
    return (mode == 0 ? unknown_04_ : unknown_08_ + unknown_04_) + unknown_02_;
  }

  short mode;         // 0x00, bits 1, 2 and 4 pick the optional children
  short unknown_02_;  // 0x02, parts of kind 0x10000 counted
  short unknown_04_;  // 0x04, parts of kind 0x20000 counted
  short progress;     // 0x06, in percent of the mode
  short unknown_08_;  // 0x08, parts of kind 0x40000 counted
};

// The state of CEnmParam that is saved with a game (0x7c bytes).
struct EnmState {
  unsigned int flags;        // 0x08
  EnmSideStats sides[2];     // 0x0c
  unsigned int color;        // 0x20
  short unknown_24_;         // 0x24
  short unknown_26_;         // 0x26
  short unknown_28_;         // 0x28
  short unknown_2a_;         // 0x2a
  short max_depth;           // 0x2c
  short unknown_2e_;         // 0x2e
  unsigned int weapon_mask;  // 0x30
  short weapon_count;        // 0x34
  char weapons[8];           // 0x36
  short weapon_levels[7];    // 0x3e
  unsigned int step_mask;    // 0x4c, the steps added so far
  short step_count;          // 0x50
  short step_total;          // 0x52
  char steps[19][2];         // 0x54
  char title[2];             // 0x7a, words of the boss's title, -1: none
  short unknown_7c_;         // 0x7c
  short unknown_7e_;         // 0x7e
  short unknown_80_;         // 0x80
  short unknown_82_;         // 0x82
};

class CEnmParam : public System {
 public:
  enum Codes {
    kCodeMask = 0x3fffffff,
    kSideMask = 0xc0000000,
  };

  CEnmParam();  // 0x419e60
  // 0x401b30, scalar deleting dtor 0x401bd0
  virtual ~CEnmParam() { Finalize(); }
  virtual int Initialize(Application* app);  // 0x41ab20
  virtual void Finalize();                   // 0x419f50
  virtual void Pause() {}
  virtual void Resume() {}

  // Starts over from the first boss.
  void Reset();  // 0x41b5b0
  // Takes the slot for the part at `code` of `kind`; NULL when full.
  EnmSlot* AllocSlot(unsigned int code, int kind);  // 0x41a2a0
  // Logs the destruction of `slot`.
  void LogDestroy(EnmSlot* slot, int chain, int rapid);  // 0x41a4c0
  // Saves / loads the state; 0 on failure.
  int Save(File* file);  // 0x41a5b0
  int Load(File* file);  // 0x41a660
  // Plans the next boss; `score` is the score so far.
  void Plan(int score);  // 0x41abc0
  void AfterPlan();      // 0x417c10, not decompiled yet
  // Writes the parameters shown on the results to `out`; returns their
  // count.
  int GetParams(short* out);  // 0x419fa0
  // Writes the results of the stage to `text`, `*columns` wide and
  // `*lines` high.
  void FormatResult(char* text, int* columns, int* lines);  // 0x41a090
  void ClearUnknown32c();                                   // 0x417c00
  // The slot of the part at `code`, NULL without one.
  EnmSlot* FindSlot(unsigned int code) {
    if (code != 0) {
      for (EnmSlot* slot = &slots_[slot_capacity_ - 1]; slot > slots_; --slot) {
        if (slot->code == code) {
          return slot;
        }
      }
    }
    return NULL;
  }
  // The code of the first child of the part at `code`.
  static unsigned int ChildBase(unsigned int code) {
    return (code & kSideMask) | (code << 3);
  }
  // The code of the parent of the part at `code`.
  static unsigned int ParentCode(unsigned int code) {
    return (code & kSideMask) | ((code & kCodeMask) >> 3);
  }

 private:
  friend class Control01;

  // Actor::destroy_hook_ while the game runs.
  static void OnActorDestroyed(Actor* source, Actor* actor);  // 0x41aa60

  void Count(EnmSlot* slot, int side);  // 0x41a190
  // Counts up to `count` logged destructions of parts of kind 1 (only those
  // destroyed alone or in a chain of 2 unless `all`; one per side when
  // `unique`), keeping the first kind of each side in `kinds`. Returns the
  // count left.
  int CountDestroyed(int count, unsigned int* kinds, int unique,
                     int all);                        // 0x41a730
  void ChooseWeapon(EnmSlot* slot, EnmSlot* parent);  // 0x41a3a0
  // Plans the part at `code` (taking a slot for it when it has none) and
  // its children.
  void Grow(EnmSlot* parent, unsigned int code, int kind);  // 0x41a820

  // Adds step `id` to the steps of the game unless it is there already.
  void AddStep(int id);

 public:
  // The controls read and change much of this directly.

  EnmState state_;       // 0x08
  int turn_speed_;       // 0x84, of the turrets, per frame
  int unknown_88_[3];    // 0x88
  int turret_count_;     // 0x94, of control 35
  int unknown_98_;       // 0x98
  EnmList list_9c_;      // 0x9c
  EnmList list_b0_;      // 0xb0
  EnmList lists_c4_[2];  // 0xc4
  // Parts of the boss in battle, per side.
  int parts_alive_[2];    // 0xec
  int unknown_f4_[2];     // 0xf4
  int armored_parts_[2];  // 0xfc, of slots with flag 8
  int unknown_104_[2];    // 0x104, with PartWork flag 0x20000
  union {
    // While planning.
    struct {
      int weapon_parts_[7];  // 0x10c, parts planned per weapon
      int power_total_;      // 0x128, of the parts planned
      int part_total_;       // 0x12c, parts planned
    };
    // In battle: the rank + 50 of the cores per side and level.
    int core_power_[2][7];  // 0x10c
  };
  int core_count_;             // 0x144
  int core_total_;             // 0x148, cores the boss started with
  int unknown_14c_;            // 0x14c
  int glow_time;               // 0x150, of the glow (control 22) of a part
  int trans_count_;            // 0x154, parts that played trans1
  int unknown_158_;            // 0x158
  int flare_time_;             // 0x15c, of control 23
  float flare_size_;           // 0x160, of control 23
  int unknown_164_;            // 0x164
  int score_base_;             // 0x168
  int score_rapid_;            // 0x16c
  int score_chain_;            // 0x170
  int chain_count_;            // 0x174
  int rapid_count_;            // 0x178
  int unknown_17c_;            // 0x17c
  int slot_capacity_;          // 0x180
  int record_capacity_;        // 0x184
  int slot_count_;             // 0x188
  int record_count_;           // 0x18c
  EnmSlot* slots_;             // 0x190
  EnmDestroyRecord* records_;  // 0x194
  EnmPlan plan_;               // 0x198
  EnmList list_218_;           // 0x218
  int unknown_22c_[64];        // 0x22c
  int unknown_32c_;            // 0x32c
  int deaths_;                 // 0x330
  int battle_time_;            // 0x334
  int unknown_338_[3];         // 0x338, picks the order of new weapons
  int unknown_344_;            // 0x344
  float unknown_348_;          // 0x348
  int unknown_34c_[2];         // 0x34c
  int unknown_354_;            // 0x354
  int new_weapon_;             // 0x358, added by Plan, or -1
  int unknown_35c_;            // 0x35c
};

extern CEnmParam g_enm_param;  // 0x4677b0

#endif  // WF_ENM_PARAM_H_
