// ActorControl: the behaviour of one kind of actor. One stateless instance
// of each subclass is registered in g_actor_system under its id by
// cGlobal::RegisterActorControls(); an actor calls the control of its id.
//
// The subclasses are named after their registration ids. Hierarchy (from the
// vtables):
//   ActorControl (vtable 0x45237c, 9 slots)
//     Control01..Control35, Control43   (9 slots)
//     ScoredControl                     (+GetScore, GetHitPoints)
//       Control45, Control48            (11 slots)
//       PartControl                     (+OnDamage 0x4198e0)
//         Control32                     (12 slots)
//         LinkedPartControl             (OnDestroy..OnChildDestroyed)
//           Control33, Control34        (12 slots)
//         CorePartControl               (+GetLevel, Setup)
//           Control40..Control49        (14 slots)
#ifndef WF_ACTOR_CONTROL_H_
#define WF_ACTOR_CONTROL_H_

class Actor;
struct ActorDesc;
struct FloatRect;
class HLSObject;
class HLSPatFile;
struct EnmSlot;
class EnmList;
class File;

// Actor::work of the parts of the boss.
struct PartWork {
  enum Flags {
    kAlive = 0x1000000,
    kTrans = 0x4000000,  // has played trans1
  };

  EnmSlot* slot;       // 0x30
  unsigned int flags;  // 0x34
  Actor* turret;       // 0x38, what turns: the part itself or its parent
  int max_hit_points;  // 0x3c
  short level;         // 0x40, the levels of the part and its children
  short low;           // 0x42, of the angles the turret may turn to
  short high;          // 0x44
  short angle;         // 0x46, where the turret starts
  int state;           // 0x48, the last event of OnDamage()
  short state_time;    // 0x4c
  short unknown_1e_;   // 0x4e
  short rank;          // 0x50, of a core: its power in percent
  short unknown_22_;   // 0x52

  // Enters `new_state`.
  void Enter(int new_state) {
    state = new_state;
    unknown_1e_ = 0;
    state_time = 0;
  }
};

// A sprite spawned on a part (0x10 bytes).
struct PartPiece {
  float x;           // 0x00
  float y;           // 0x04
  char layer;        // 0x08, relative to the part's
  char debris;       // 0x09, an index into g_debris_controls
  short pat;         // 0x0a
  HLSPatFile* file;  // 0x0c
};

// How control 8 (an effect sprite) moves and fades (0x34 bytes).
struct EffectDesc {
  HLSObject* sprite;     // 0x00
  unsigned short flags;  // 0x04, added to Actor::flags
  unsigned short life;   // 0x06, frames, 0: until it fades or shrinks away
  float scale_min;       // 0x08
  float scale_max;       // 0x0c
  float grow;            // 0x10, added to the scale every frame
  float speed_min;       // 0x14
  float speed_max;       // 0x18
  float acceleration;    // 0x1c
  short angle_min;       // 0x20
  short angle_max;       // 0x22
  short color[4];        // 0x24, red, green, blue, alpha
  short fade[4];         // 0x2c, added to the color every frame
};

// The controls of the cores by weapon (0x45d91c).
extern int g_core_controls[7];
// The debris controls by size (0x45d938).
extern int g_debris_controls[4];

class ActorControl {
 public:
  ActorControl() {}
  virtual ~ActorControl();  // 0x40fef0, scalar deleting dtor 0x40fe10
  // Called once when registered; 0 rejects the control.
  virtual int Initialize() = 0;
  virtual void Finalize() = 0;
  // Returns 0 when the actor could not be set up.
  virtual int OnCreate(Actor* actor, void* param) = 0;
  virtual void OnDestroy(Actor* actor) = 0;
  // Returns 0 when the actor is done.
  virtual int OnMove(Actor* actor) = 0;
  // 0x40fde0
  virtual void OnEvent(Actor* actor, int x, int y) {}
  // 0x40fdf0
  virtual void OnChildDestroyed(Actor* parent, Actor* child) {}
  // 0x40fe00
  virtual int OnMessage(Actor* actor, int message, int param) { return 0; }
};

class ScoredControl : public ActorControl {
 public:
  virtual int GetScore(Actor* actor) = 0;
  virtual int GetHitPoints(Actor* actor) = 0;
  // Sets the actor up as a part planned in `param` (an EnmSlot).
  int Prepare(Actor* actor, void* param);  // 0x418c40
  // 1 when the actor is outside the screen.
  int IsOffScreen(Actor* actor);  // 0x4146a0
};

class PartControl : public ScoredControl {
 public:
  virtual void OnDestroy(Actor* actor);          // 0x4195f0
  virtual void OnDamage(Actor* actor, int how);  // 0x4198e0
  // Scales the hit points by the level of the part.
  void ScaleHitPoints(Actor* actor);  // 0x418d70
  // Sets the actor up as the part planned in `param` (an EnmSlot).
  int InitPart(Actor* actor, void* param);  // 0x419070
  // Spawns `count` sprites from `pieces` on the part.
  int SpawnPieces(Actor* actor, const PartPiece* pieces,
                  int count);  // 0x418f50
  // Spawns the items of the part template.
  int SpawnItems(Actor* actor);  // 0x4194a0
  // Plays trans1 the first time.
  void PlayTrans(PartWork* work);  // 0x419020
  // Stops trans1 when no part plays it any more; then plays trans2 when
  // `next`.
  void StopTrans(PartWork* work, int next);  // 0x419580
  // Waits `delay` frames (state 1), then calls OnDamage(actor, `next`).
  void Schedule(Actor* actor, int delay, int next);  // 0x418c90
  // Runs the states OnDamage() sets; returns 1.
  int MovePart(Actor* actor);  // 0x419a70
  // Spawns control 23 on the part, pointing away from the boss, unless the
  // part is within a quarter turn of CEnmParam::unknown_158_ from it.
  void SpawnFlare(Actor* actor);  // 0x419850
};

class LinkedPartControl : public PartControl {
 public:
  virtual int OnMove(Actor* actor);                            // 0x419cc0
  virtual void OnEvent(Actor* actor, int x, int y);            // 0x419370
  virtual void OnChildDestroyed(Actor* parent, Actor* child);  // 0x419400
  // 0x40fe30
  virtual int GetScore(Actor* actor);
  // 0x40fe40
  virtual int GetHitPoints(Actor* actor);
  // Spawns the glow (control 22) on `actor`; 0 when it cannot.
  int SpawnGlow(Actor* actor);  // 0x418ec0
  // InitPart() plus the look of the part.
  int InitLinked(Actor* actor, void* param);  // 0x4192b0
  // Spawns the `count` children at `base` | `index`... of the part; 0 when
  // one was not planned.
  int SpawnChildren(Actor* actor, unsigned int base, unsigned int index,
                    int count);  // 0x418da0
};

class CorePartControl : public PartControl {
 public:
  virtual void OnDestroy(Actor* actor);              // 0x4197d0
  virtual void OnEvent(Actor* actor, int x, int y);  // 0x418cc0
  virtual int GetLevel() = 0;
  virtual void Setup(Actor* actor, int rank) = 0;
  // InitPart() plus the rank of the core.
  int InitCore(Actor* actor, void* param);  // 0x419220

  // The largest PartWork::unknown_22_ of a core at event 5.
  static int max_unknown_22_;  // 0x469274
};

// Simple controls ---------------------------------------------------------

// The player.
class Control01 : public ActorControl {
 public:
  enum Weapons { kShot, kLaser, kWeapons };
  enum Events { kEventInvincible = 1 };

  virtual int Initialize();  // 0x412fe0
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x413550
  virtual void OnDestroy(Actor* actor);             // 0x412fa0
  virtual int OnMove(Actor* actor);                 // 0x4136f0
  // kEventInvincible: invincible for `value` frames (for good when
  // negative, no longer when 0).
  virtual void OnEvent(Actor* actor, int event, int value);  // 0x4130f0

  // Starts the next game with the shot.
  static void ResetWeapon();  // 0x412f90
  // Write / read the state of the player for a replay.
  static int Save(Actor* actor, File* file);  // 0x413070
  static int Load(Actor* actor, File* file);  // 0x413440

 private:
  void Fire(Actor* actor);  // 0x413130
  // Switches to `weapon` (the next one when negative).
  static void SelectWeapon(Actor* actor, int weapon, float x, float y);

  static int weapon_;      // 0x4690ec, kept from one life to the next
  static ActorDesc shot_;  // 0x4690b8
};

class Control08 : public ActorControl {
 public:
  virtual int Initialize();  // 0x412c80
  virtual void Finalize() {}
  // `param` is an EffectDesc, NULL for default_.
  virtual int OnCreate(Actor* actor, void* param);  // 0x412cb0
  virtual void OnDestroy(Actor* actor) {}
  virtual int OnMove(Actor* actor);  // 0x412e70

 private:
  static EffectDesc default_;  // 0x45d458
};

class Control16 : public ActorControl {
 public:
  virtual int Initialize();  // 0x410540
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x410640
  virtual void OnDestroy(Actor* actor);             // 0x4104d0
  virtual int OnMove(Actor* actor);                 // 0x4104e0
};

// Controls whose Initialize() only returns 1 (0x410340).
#define WF_SIMPLE_CONTROL(name)                      \
  class name : public ActorControl {                 \
   public:                                           \
    virtual int Initialize() { return 1; }           \
    virtual void Finalize() {}                       \
    virtual int OnCreate(Actor* actor, void* param); \
    virtual void OnDestroy(Actor* actor) {}          \
    virtual int OnMove(Actor* actor);                \
  }

WF_SIMPLE_CONTROL(Control17);  // OnCreate 0x410930, OnMove 0x410500
WF_SIMPLE_CONTROL(Control18);  // OnCreate 0x410a00, OnMove 0x410a80
WF_SIMPLE_CONTROL(Control19);  // OnCreate 0x410be0, OnMove 0x410710
WF_SIMPLE_CONTROL(Control21);  // OnCreate 0x4148d0, OnMove 0x414940
WF_SIMPLE_CONTROL(Control22);  // OnCreate 0x410470, OnMove 0x410350
WF_SIMPLE_CONTROL(Control23);  // OnCreate 0x411380, OnMove 0x411450

#undef WF_SIMPLE_CONTROL

class Control20 : public ActorControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x410d50
  virtual void OnDestroy(Actor* actor);             // 0x410520
  virtual int OnMove(Actor* actor);                 // 0x410e70
};

class Control35 : public ActorControl {
 public:
  virtual int Initialize();  // 0x412820
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x412950
  virtual void OnDestroy(Actor* actor);             // 0x4127c0
  virtual int OnMove(Actor* actor);                 // 0x4129c0

 private:
  // Starts a burst of bullets in a random direction.
  void StartBurst(Actor* actor);  // 0x4127d0
  // Fires a ring of 16 bullets once few cores are left.
  void FireRing(Actor* actor);  // 0x412880

  static ActorDesc desc_;  // 0x469080
};

class Control43 : public ActorControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);   // 0x414210
  virtual void OnDestroy(Actor* actor);              // 0x414430
  virtual int OnMove(Actor* actor);                  // 0x414480
  virtual void OnEvent(Actor* actor, int x, int y);  // 0x414550

  // The glow at the muzzle, and the segments of the beam (Control42
  // fills them in).
  static ActorDesc glow_desc_;  // 0x469198
  static ActorDesc beam_desc_;  // 0x4691cc

 private:
  // Actors holding the sound channels 19 and 20.
  static int sound_users_[2];  // 0x469200
};

// Scored controls ---------------------------------------------------------

class Control45 : public ScoredControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x414830
  virtual void OnDestroy(Actor* actor) {}
  virtual int OnMove(Actor* actor);  // 0x414ac0
  virtual int GetScore(Actor* actor) { return 100; }
  virtual int GetHitPoints(Actor* actor) { return 2; }
};

class Control48 : public ScoredControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x413d50
  virtual void OnDestroy(Actor* actor) {}
  virtual int OnMove(Actor* actor);  // 0x413c30
  virtual int GetScore(Actor* actor) { return 100; }
  virtual int GetHitPoints(Actor* actor) { return 5; }
};

// Actor::work of control 32, the body of the boss.
struct Control32Work {
  enum Flags {
    kDashing = 0x1,  // in PartWork::flags
    kTurning = 0x2,
  };

  char part_[0x20];    // 0x30, as PartWork (only its first 0x20 bytes)
  Actor* arms[4];      // 0x50, the parts on the left (0, 1) and right (2, 3)
  float deceleration;  // 0x60, of the dash
  short brake_time;    // 0x64, frames into the dash it starts to brake, 0:
                       // braking
  // A turn (RunProgramB0): it speeds up until turn_peak, then slows down
  // until turn_end, turning by `turn` from `turn_from`.
  unsigned short turn_from;  // 0x66
  short turn;                // 0x68
  short turn_peak;           // 0x6a, frames into the turn
  short turn_end;            // 0x6c
};

class Control32 : public LinkedPartControl {
 public:
  virtual int Initialize();  // 0x411500
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);             // 0x4116f0
  virtual void OnDestroy(Actor* actor);                        // 0x411550
  virtual int OnMove(Actor* actor);                            // 0x412750
  virtual void OnEvent(Actor* actor, int x, int y);            // 0x411d10
  virtual void OnChildDestroyed(Actor* parent, Actor* child);  // 0x412400
  virtual int GetScore(Actor* actor);                          // 0x411530
  virtual int GetHitPoints(Actor* actor) { return 1; }

  // The body of the boss while it is alive.
  static Actor* boss_;  // 0x469270

 private:
  // Starts a dash toward (`x`, `y`) (kept within bounds_) that stops
  // there; `boost` flares the parts. Returns 0 when it is there already.
  int Dash(Actor* actor, EnmList* program, float x, float y, float power,
           int boost);  // 0x411e20
  // Run the movement programs of CEnmParam.
  void RunProgram9c(Actor* actor, EnmList* program);              // 0x4120d0
  void RunProgramB0(Actor* actor, EnmList* program);              // 0x412530
  void RunSideProgram(Actor* actor, EnmList* program, int side);  // 0x4119d0
  // Starts a turn of `actor` to `angle`; 0 when it faces it already.
  int StartTurn(Actor* actor, EnmList* program,
                int angle);  // 0x411580
  // Writes the weapons that the cores of `side` still have to `out`;
  // returns their count.
  int ListWeapons(int side, int* out);  // 0x411600
  // Sends the event (`x`, `y`) to the parts of `side`.
  void SignalSide(Actor* actor, int side, int x, int y);  // 0x411960
  // The angle from `actor` to the player (or to where it starts).
  unsigned short AngleToPlayer(Actor* actor);  // 0x4124f0

 private:
  // Where the body may move.
  static FloatRect bounds_;  // 0x469054
  // Scratch space of RunSideProgram for ListWeapons.
  static int weapons_[7];  // 0x469064
};

class Control33 : public LinkedPartControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x412c30
};

class Control34 : public LinkedPartControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x412b90
};

// Core part controls: score, hit points and level are constants. Setup()
// derives the weapon's parameters from the rank.
#define WF_CORE_CONTROL_BODY(score, hit_points, level)          \
 public:                                                        \
  virtual void Finalize() {}                                    \
  virtual int OnCreate(Actor* actor, void* param);              \
  virtual int OnMove(Actor* actor);                             \
  virtual int GetScore(Actor* actor) { return score; }          \
  virtual int GetHitPoints(Actor* actor) { return hit_points; } \
  virtual int GetLevel() { return level; }                      \
  virtual void Setup(Actor* actor, int rank);

// Bullets (spawns 13 bullets at a time).
class Control40 : public CorePartControl {
  WF_CORE_CONTROL_BODY(1000, 20, 0)
  virtual int Initialize();  // 0x413a40
  // OnCreate 0x413aa0, OnMove 0x413b00, Setup 0x4139c0

 private:
  static ActorDesc desc_;  // 0x469130
};

// A fan of shells.
class Control41 : public CorePartControl {
  WF_CORE_CONTROL_BODY(1200, 30, 1)
  virtual int Initialize();  // 0x413e30
  // OnCreate 0x413ea0, OnMove 0x413f00, Setup 0x413dc0

 private:
  static ActorDesc desc_;  // 0x469164
};

// A laser (control 43).
class Control42 : public CorePartControl {
  WF_CORE_CONTROL_BODY(1200, 30, 2)
  virtual int Initialize();  // 0x414040
  // OnCreate 0x4140d0, OnMove 0x414130, Setup 0x413fc0
};

// Homing missiles (control 45).
class Control44 : public CorePartControl {
  WF_CORE_CONTROL_BODY(2000, 40, 3)
  virtual int Initialize() { return 1; }
  // OnCreate 0x414700, OnMove 0x414760, Setup 0x414650
};

// Slow missiles (control 48).
class Control47 : public CorePartControl {
  WF_CORE_CONTROL_BODY(2000, 40, 5)
  virtual int Initialize() { return 1; }
  // OnCreate 0x413c60, OnMove 0x413cc0, Setup 0x413be0
};

// A spray of sparks.
class Control46 : public CorePartControl {
  WF_CORE_CONTROL_BODY(2500, 40, 4)
  virtual int Initialize();  // 0x414ba0
  // OnCreate 0x414c10, OnMove 0x414c70, Setup 0x414b40

 private:
  static ActorDesc desc_;  // 0x469208
};

// A sweeping stream of bullets.
class Control49 : public CorePartControl {
  WF_CORE_CONTROL_BODY(2500, 40, 6)
  virtual int Initialize();  // 0x414e20
  // OnCreate 0x414e80, OnMove 0x414f20, Setup 0x414dc0

 private:
  static ActorDesc desc_;  // 0x46923c
};

#undef WF_CORE_CONTROL_BODY

#endif  // WF_ACTOR_CONTROL_H_
