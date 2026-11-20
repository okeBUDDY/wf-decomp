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
  short unused_10_;    // 0x10, 8 in every entry of plan.dat, not read
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

  char name[12];            // 0x1c, an editor label ("ATK-ROOT", "NEW"), not
                            // read
  int score;                // 0x28, of a linked part
  short hit_points;         // 0x2c, of a linked part
  short x1;                 // 0x2e, where it is while armed (flags & 3 == 1)
  short y1;                 // 0x30
  short unused_32_;         // 0x32, 9 in every part of plan.dat, not read
  short lerp_from;          // 0x34
  short lerp_to;            // 0x36
  short child_count;        // 0x38
  short option_count;       // 0x3a, children added in mode 1 or 2
  short option4_count;      // 0x3c, children added in mode 4
  short extra_child_count;  // 0x3e
  short item_count;         // 0x40
                            // 0x42: padding (0 in plan.dat)
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
  int version_;           // 0x04, 1 in plan.dat (not checked)
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

// A program the boss runs each frame (vtable 0x4514b0, 0x14 bytes): the
// operation, its argument and how far it got. CEnmParam runs a tactic in
// tactic_program_ (op = the tactic id), and the tactics start the move,
// turn and fire programs that BossBodyControl runs on the body.
class EnmProgram {
 public:
  EnmProgram() {  // 0x419e40
    time = 0;
    step = 0;
    arg = 0;
    op = 0;
  }
  // 0x401a90, scalar deleting dtor 0x401aa0
  virtual ~EnmProgram() {}

  // Starts operation `new_op` with `new_arg`.
  void Start(int new_op, int new_arg) {
    op = new_op;
    arg = new_arg;
    step = 0;
    time = 0;
  }

  // Waits `frames` frames (time counts down) and goes on to the next step.
  // The store through a pointer keeps VC from tracking the step across it:
  // the original increments the step in memory right after the store.
  void Wait(int frames) {
    int* timer = &time;
    *timer = frames;
    ++step;
  }
  // Sets the argument and goes on to the next step (as Wait).
  void SetArgAndNext(int new_arg) {
    int* value = &arg;
    *value = new_arg;
    ++step;
  }

  // The operations of the move, turn and fire programs.
  enum Ops {
    kMoveEnter = 0x10001,   // comes down onto the screen
    kMoveRandom = 0x10002,  // dashes to a random place
    kMoveCharge = 0x10003,  // dashes straight ahead
    kMoveCenter = 0x10004,  // dashes to the top center
    kMoveRam = 0x10005,     // dashes at the player
    kMoveRecoil = 0x10006,  // knocked back to angle `arg`
    kTurnFlip = 0x20001,    // half a turn either way
    kTurnTo = 0x20002,      // turns to angle `arg`
    kTurnSpin = 0x20003,    // spins for about `arg` frames
    kTurnFace = 0x20004,    // faces the player (offset by `arg`)
    kFireAny = 0x30001,     // fires random weapons, `arg` percent of the rate
    kFireWeapon = 0x30002,  // fires weapon `arg` once
    kFireWeaponWait = 0x30003,  // ... and waits for the volley
    kFireBoosted = 0x30004,     // fires weapon `arg` at full rank once
    kFireBoostedWait = 0x30005,
    kFireAll = 0x30006,  // fires every weapon once
    kFireAllWait = 0x30007,
  };

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
    if (mode == 0) {
      return wide_kills + attack_kills;
    }
    return rapid_kills + wide_kills + attack_kills;
  }

  short mode;          // 0x00, bits 1, 2 and 4 (attack, wide, rapid) pick the
                       // optional children
  short attack_kills;  // 0x02, parts of kind 0x10000 (ATK-ROOT) counted
  short wide_kills;    // 0x04, parts of kind 0x20000 (WIDE-ROOT) counted
  short progress;      // 0x06, in percent of the mode
  short rapid_kills;   // 0x08, parts of kind 0x40000 counted, and rapid
                       // destructions while the mode is 0
};

// The title of a boss (8 bytes).
struct EnmTitle {
  char words[2];  // 0x00, -1: none
  short weapon;   // 0x02, the main weapon, 7: mixed, -1: none
  short level;    // 0x04, of the weapon (at most 20), or the weapon count
  short variant;  // 0x06, counts up each time the title comes again
};

// The state of CEnmParam that is saved with a game (0x7c bytes).
struct EnmState {
  enum Flags {
    // Cleared every frame (by RunBattle()).
    kAimed = 0x1,   // a turret is on target
    kTurned = 0x2,  // the turn program is done or slowing down
    // Kept.
    kBattle = 0x100,  // the body has come down; the tactics run
    // Set while planning.
    kArmor = 0x10000,      // a part with plan flag 0x100 (armored) is planned
    kTransform = 0x20000,  // a part with plan flag 0x20 (transforms)
  };

  unsigned int flags;        // 0x08, Flags
  EnmSideStats sides[2];     // 0x0c
  unsigned int color;        // 0x20
  short body_hit_points;     // 0x24, of control 32, 200 up to 1200
  short toughness;           // 0x26, 0..0x400, adds toughness / 1024 to the hit
                             // points of the parts
  short fire_speed;          // 0x28, 0..0x400, gives CEnmParam::fire_rate_
  short aim_speed;           // 0x2a, 0..0x400, gives CEnmParam::turn_speed_
  short max_depth;           // 0x2c
  short unused_2e_;          // 0x2e, never used
  unsigned int weapon_mask;  // 0x30
  short weapon_count;        // 0x34
  char weapons[8];           // 0x36
  short weapon_levels[7];    // 0x3e
  unsigned int tactic_mask;  // 0x4c, bit i: tactic i is available
  short tactic_count;        // 0x50
  short tactic_weight;       // 0x52, the sum of the weights
  char tactics[19][2];       // 0x54, the tactic ids and their running
                             // weight sums, for the pick in RunBattle()
  EnmTitle title;            // 0x7a
  short unused_82_;          // 0x82, never used
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
  // Chooses the title of the planned boss from the modes of its sides and
  // its weapons.
  void ChooseTitle();  // 0x417c10
  // Writes the title of the boss to `out`, in Japanese, broken into two
  // lines when long.
  void FormatTitleJa(char* out);  // 0x418030
  // The same in English, broken where it is wider than 240 pixels in
  // g_fonts.title.
  void FormatTitleEn(char* out);  // 0x418400
  // Writes the parameters shown on the results to `out`; returns their
  // count.
  int GetParams(short* out);  // 0x419fa0
  // The size of the buffer FormatResult() needs.
  int GetResultSize();  // 0x41a080
  // Writes the results of the stage to `text`, `*columns` wide and
  // `*lines` high.
  void FormatResult(char* text, int* columns, int* lines);  // 0x41a090
  // Forgets the titles given so far.
  void ClearTitles();  // 0x417c00

  // The battle tactics, run by tactic_program_ on the boss; each returns 1
  // when it is over. RunBattle() picks them by id (the op of
  // tactic_program_; ids 12 and 13 are empty).
  // Nonzero when the stronger side (by the percentage of its parts alive)
  // is the one player_dir_ does not name, and the weaker one is down to
  // `percent`.
  int IsSideBeaten(Actor* boss, int percent);  // 0x416170
  // 0: dashes about at random, now and then at the player.
  int TacticRoam(Actor* boss);  // 0x4161d0
  // 1: dashes about; turns half round when a side is beaten to 50%.
  int TacticDashFlip(Actor* boss);  // 0x4162f0
  // The opening that tactics 2, 9 and 10 share (steps 0 to 3: faces the
  // player while firing, then charges), then goes on at `next_step`.
  void RunOpening(Actor* boss, int next_step);  // 0x4163f0
  // 2: the opening, then a dash and a turn back.
  int TacticCharge(Actor* boss);  // 0x416560
  // 3: turns its back on the player and fires the sparks of side 1
  // (weapon 4) at full rank, 3 to 5 times.
  int TacticRearSparks(Actor* boss);  // 0x416630
  // 4: fires the streams (weapon 6) of both sides while dashing about.
  int TacticStream(Actor* boss);  // 0x4167a0
  // 5: fires at twice the rate while dashing about for 5 to 15 seconds.
  int TacticRapidFire(Actor* boss);  // 0x4168e0
  // 7: spins for 20 to 40 seconds while dashing about.
  int TacticSpinDash(Actor* boss);  // 0x4169c0
  // 9: the opening, then tactic 3.
  int TacticChargeRearSparks(Actor* boss);  // 0x416ac0
  // 10: the opening, then half a turn at twice the fire rate.
  int TacticChargeFlip(Actor* boss);  // 0x416cb0
  // 11: faces the player and charges, 3 to 5 times.
  int TacticRepeatCharge(Actor* boss);  // 0x416e10
  // 14: spins for 20 to 40 seconds at twice the fire rate.
  int TacticSpin(Actor* boss);  // 0x417010
  // 18: once the body has lost its parts: turns back and dashes for good.
  int TacticLastStand(Actor* boss);  // 0x4170c0
  // 6: the armored parts glow for 5 to 10 seconds while it dashes about.
  int TacticGlowArmor(Actor* boss);  // 0x417430
  // 8: transforms the parts (event 4) and waits until they are done.
  int TacticTransform(Actor* boss);  // 0x417520
  // 15: glows and spins; side 1 fires its sparks.
  int TacticSpinGlowSparks(Actor* boss);  // 0x4175d0
  // 16: glows and fires at twice the rate while dashing about.
  int TacticGlowRapidFire(Actor* boss);  // 0x4176f0
  // 17: glows and spins while ramming the player.
  int TacticSpinRam(Actor* boss);  // 0x4177e0
  // Sets up the battle with the planned boss and creates it (control 32).
  Actor* StartBattle(Actor* player);  // 0x417170
  // Runs a frame of the battle: follows `player` and runs the tactics on
  // `boss`, choosing the next one when a tactic is over.
  void RunBattle(Actor* player, Actor* boss);  // 0x4178f0
  // Makes the armored parts glow for `time` frames (sending event 8 to
  // `actor`); 0 without armored parts.
  int GlowArmor(Actor* actor, int time);  // 0x4173a0
  // Sends event 4 to `actor` when parts with PartWork flag 0x20000 are
  // left; 0 without them.
  int StartTransform(Actor* actor);  // 0x4173f0
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
  friend class PlayerControl;

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
  // Adds the part in `slot` to the planned totals.
  void CountPart(const EnmSlot* slot) {
    ++weapon_parts_[slot->weapon];
    ++part_total_;
    power_total_ += slot->power;
  }

  // Makes tactic `id` available unless it is already.
  void AddTactic(int id);

 public:
  // The controls read and change much of this directly.

  EnmState state_;           // 0x08
  int turn_speed_;           // 0x84, of the turrets, per frame
  int fire_rate_;            // 0x88, the fire waits count down by it
  int fire_wait_[2];         // 0x8c, per side, before the next volley
  int turret_count_;         // 0x94, of control 35
  int turret_weight_;        // 0x98, the bursts of a turret: (this - depth) * 4
  EnmProgram move_program_;  // 0x9c, EnmProgram::kMove...
  EnmProgram turn_program_;  // 0xb0, EnmProgram::kTurn...
  EnmProgram fire_programs_[2];  // 0xc4, per side, EnmProgram::kFire...
  // Parts of the boss in battle, per side.
  int parts_alive_[2];      // 0xec
  int start_parts_[2];      // 0xf4, parts_alive_ when the battle started
  int armored_parts_[2];    // 0xfc, of slots with flag 8
  int transform_parts_[2];  // 0x104, with PartWork::kTransforms
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
  int core_count_;  // 0x144
  int core_total_;  // 0x148, cores the boss started with
  // Where the player is seen from the boss: 0 behind it (within 0x40 of
  // 0x200), 1 at its side (within 0x100), 2 in front.
  int player_dir_;             // 0x14c
  int glow_time_;              // 0x150, of the glow (control 22) of a part
  int trans_count_;            // 0x154, parts that played trans1
  int dash_angle_;             // 0x158, of the last boosted dash
  int flare_time_;             // 0x15c, of control 23
  float flare_size_;           // 0x160, of control 23
  int flare_count_;            // 0x164, flares since the boosted dash
  int score_base_;             // 0x168
  int score_rapid_;            // 0x16c
  int score_chain_;            // 0x170
  int chain_count_;            // 0x174
  int rapid_count_;            // 0x178
  int unused_17c_;             // 0x17c, cleared by StartBattle(), not read
  int slot_capacity_;          // 0x180
  int record_capacity_;        // 0x184
  int slot_count_;             // 0x188
  int record_count_;           // 0x18c
  EnmSlot* slots_;             // 0x190
  EnmDestroyRecord* records_;  // 0x194
  EnmPlan plan_;               // 0x198
  EnmProgram tactic_program_;  // 0x218, op: the tactic id
  EnmTitle titles_[32];        // 0x22c, given so far
  int title_count_;            // 0x32c
  int deaths_;                 // 0x330
  int battle_time_;            // 0x334
  int dir_frames_[3];    // 0x338, frames by player_dir_; pick the order of new
                         // weapons
  int aimed_frames_;     // 0x344, frames with EnmState::kAimed
  float player_travel_;  // 0x348, sampled every 32 frames
  float player_pos_[2];  // 0x34c, when last sampled
  int player_samples_;   // 0x354
  int new_weapon_;       // 0x358, added by Plan, or -1
  int unused_35c_;       // 0x35c, never used
};

extern CEnmParam g_enm_param;  // 0x4677b0

#endif  // WF_ENM_PARAM_H_
