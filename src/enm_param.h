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

// A part template of plan.dat (0x48 bytes).
struct EnmPlanPart {
  unsigned int code;         // 0x00, the position in the part tree
  unsigned int flags;        // 0x04
  unsigned int kind;         // 0x08
  int unknown_0c_;           // 0x0c
  short unknown_10_;         // 0x10
  short low;                 // 0x12
  short high;                // 0x14
  short unknown_16_[15];     // 0x16
  short lerp_from;           // 0x34
  short lerp_to;             // 0x36
  int unknown_38_[3];        // 0x38
  struct EnmPlanItem* item;  // 0x44, an index into EnmPlan::items_ on disk
};

// A weapon or extra of plan.dat (0x1c bytes).
struct EnmPlanItem {
  unsigned int code;   // 0x00
  int unknown_04_[6];  // 0x04
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
class EnmList {
 public:
  EnmList() {  // 0x419e40
    unknown_08_ = 0;
    unknown_10_ = 0;
    unknown_04_ = 0;
    unknown_0c_ = 0;
  }
  // 0x401a90, scalar deleting dtor 0x401aa0
  virtual ~EnmList() {}

  int unknown_04_;
  int unknown_08_;
  int unknown_0c_;
  int unknown_10_;
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
  short how;      // 0x04
  short chain;    // 0x06
};

// Destructions counted for one side of the boss (10 bytes).
struct EnmSideStats {
  int Score() const {
    return (unknown_00_ == 0 ? unknown_04_ : unknown_08_ + unknown_04_) +
           unknown_02_;
  }

  short unknown_00_;
  short unknown_02_;
  short unknown_04_;
  short unknown_06_;
  short unknown_08_;
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
  unsigned int unknown_4c_;  // 0x4c
  short step_count;          // 0x50
  short step_total;          // 0x52
  char steps[24][2];         // 0x54
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
  void LogDestroy(EnmSlot* slot, int how, int chain);  // 0x41a4c0
  // Saves / loads the state; 0 on failure.
  int Save(File* file);  // 0x41a5b0
  int Load(File* file);  // 0x41a660
  void Plan(int stage);  // 0x41abc0
  // Writes the parameters shown on the results to `out`; returns their
  // count.
  int GetParams(short* out);  // 0x419fa0
  // Writes the results of the stage to `text`, `*columns` wide and
  // `*lines` high.
  void FormatResult(char* text, int* columns, int* lines);  // 0x41a090
  void ClearUnknown32c();                                   // 0x417c00

 private:
  // Actor::destroy_hook_ while the game runs.
  static void OnActorDestroyed(Actor* source, Actor* actor);  // 0x41aa60

  void Count(EnmSlot* slot, int side);  // 0x41a190
  // Counts up to `count` logged destructions of parts of kind 1 (only those
  // destroyed by the first two ways unless `all`; one per side when
  // `unique`), keeping the first kind of each side in `kinds`. Returns the
  // count left.
  int CountDestroyed(int count, unsigned int* kinds, int unique,
                     int all);                        // 0x41a730
  void ChooseWeapon(EnmSlot* slot, EnmSlot* parent);  // 0x41a3a0

  EnmState state_;             // 0x08
  int unknown_84_[4];          // 0x84
  int unknown_94_;             // 0x94
  int unknown_98_;             // 0x98
  EnmList list_9c_;            // 0x9c
  EnmList list_b0_;            // 0xb0
  EnmList lists_c4_[2];        // 0xc4
  int unknown_ec_[31];         // 0xec
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
  int unknown_338_[8];         // 0x338
  int unknown_358_;            // 0x358
  int unknown_35c_;            // 0x35c
};

extern CEnmParam g_enm_param;  // 0x4677b0

#endif  // WF_ENM_PARAM_H_
