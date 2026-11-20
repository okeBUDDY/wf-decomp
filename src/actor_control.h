// ActorControl: the behaviour of one kind of actor. One stateless instance
// of each subclass is registered in g_actor_system under its id by
// cGlobal::RegisterActorControls(); an actor calls the control of its id.
//
// The registration ids (Actor::id, the first argument of
// ActorSystem::Create()) of the subclasses. Hierarchy (from the vtables):
//   ActorControl (vtable 0x45237c, 9 slots)
//     PlayerControl 1, EffectControl 8, PlayerExplosionControl 16,
//     Small/Medium/LargeExplosionControl 17/18/19, BossExplosionControl 20,
//     SmokeControl 21, GlowControl 22, FlareControl 23, TurretControl 35,
//     LaserControl 43                                     (9 slots)
//     ScoredControl                   (+GetScore, GetHitPoints)
//       HomingMissileControl 45, MissileControl 48       (11 slots)
//       PartControl                   (+OnDamage 0x4198e0)
//         LinkedPartControl           (OnDestroy..OnChildDestroyed)
//           BossBodyControl 32, BossArmControl 33,
//           JointPartControl 34                          (12 slots)
//         CorePartControl             (+GetLevel, Setup)
//           BulletCoreControl 40, ShellCoreControl 41, LaserCoreControl 42,
//           HomingCoreControl 44, SparkCoreControl 46, MissileCoreControl 47,
//           StreamCoreControl 49                         (14 slots)
#ifndef WF_ACTOR_CONTROL_H_
#define WF_ACTOR_CONTROL_H_

class Actor;
struct ActorDesc;
struct FloatRect;
class HLSObject;
class HLSPatFile;
struct EnmSlot;
class EnmProgram;
class File;

// Actor::work of the parts of the boss.
struct PartWork {
  enum Flags {
    // Bits 0..15 are the control's own (BossBodyWork::Flags).
    kAims = 0x10000,           // takes kEventAim/kEventAimFire (armed parts)
    kTransforms = 0x20000,     // takes kEventTransform (plan flag 0x20)
    kArmored = 0x40000,        // takes kEventGlow (EnmSlot flag 8)
    kReady = 0x1000000,        // in kEventIdle (or aiming), takes events
    kBoosted = 0x2000000,      // set up at rank 100 for this volley
    kTrans = 0x4000000,        // has played trans1
    kTransformed = 0x8000000,  // turned to the far angle by kEventTransform
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
  short step;          // 0x4c, within the state; in kEventWait, the event
                       // to take when `timer` runs out
  short timer;         // 0x4e, frames, counting down (or up) in the state
  short rank;          // 0x50, of a core: its power in percent
  short volley_time;   // 0x52, of a core: frames a volley lasts

  // Enters `new_state`.
  void Enter(int new_state) {
    state = new_state;
    timer = 0;
    step = 0;
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
  // The events of OnDamage() (the `x` of OnEvent()); the part enters the
  // state of the same number. The `y` of OnEvent() selects the parts:
  // bits 0..7 the depth to pass the event down to, 0x100 any depth,
  // 0x200 / 0x400 skip side 1 / 0 (BossBodyControl), 0x800 at rank 100 (cores),
  // 0x1000 linked parts, 0x2000 also parts that are not kReady, and
  // 0x10000 << level the cores of each weapon level.
  enum Events {
    kEventIdle = 0,       // stays at its angle
    kEventWait = 1,       // waits PartWork::timer frames, then takes `step`
    kEventReturn = 3,     // turns back to its start angle, then idles
    kEventTransform = 4,  // turns to the far end of its range (and back)
    kEventFire = 5,       // a core fires a volley
    kEventAim = 6,        // turns to follow the player
    kEventAimFire = 7,    // the same, firing once it is on target
    kEventGlow = 8,       // an armored part glows (CEnmParam::GlowArmor)
    kEventFlare = 9,      // flares (CEnmParam::dash_angle_)
  };

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
  // part is within a quarter turn of CEnmParam::dash_angle_ from it.
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

  // The longest PartWork::volley_time of the cores that took kEventFire
  // since RunFireProgram() cleared it.
  static int max_volley_time_;  // 0x469274
};

// Simple controls ---------------------------------------------------------

// Control 1: the player.
class PlayerControl : public ActorControl {
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

  static ActorDesc shot_;  // 0x4690b8
};

// Control 8.
class EffectControl : public ActorControl {
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

// Control 16.
class PlayerExplosionControl : public ActorControl {
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

// Control 17; OnCreate 0x410930, OnMove 0x410500.
WF_SIMPLE_CONTROL(SmallExplosionControl);
// Control 18; OnCreate 0x410a00, OnMove 0x410a80.
WF_SIMPLE_CONTROL(MediumExplosionControl);
// Control 19; OnCreate 0x410be0, OnMove 0x410710.
WF_SIMPLE_CONTROL(LargeExplosionControl);
// Control 21, the smoke of a missile; OnCreate 0x4148d0, OnMove 0x414940.
WF_SIMPLE_CONTROL(SmokeControl);
// Control 22, the glow of an armored part; OnCreate 0x410470, OnMove
// 0x410350.
WF_SIMPLE_CONTROL(GlowControl);
// Control 23, the flare of a part; OnCreate 0x411380, OnMove 0x411450.
WF_SIMPLE_CONTROL(FlareControl);

#undef WF_SIMPLE_CONTROL

// Control 20.
class BossExplosionControl : public ActorControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x410d50
  virtual void OnDestroy(Actor* actor);             // 0x410520
  virtual int OnMove(Actor* actor);                 // 0x410e70
};

// Control 35.
class TurretControl : public ActorControl {
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

// Control 43.
class LaserControl : public ActorControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);   // 0x414210
  virtual void OnDestroy(Actor* actor);              // 0x414430
  virtual int OnMove(Actor* actor);                  // 0x414480
  virtual void OnEvent(Actor* actor, int x, int y);  // 0x414550

  // The glow at the muzzle, and the segments of the beam (LaserCoreControl
  // fills them in).
  static ActorDesc glow_desc_;  // 0x469198
  static ActorDesc beam_desc_;  // 0x4691cc

 private:
  // Actors holding the sound channels 19 and 20.
  static int sound_users_[2];  // 0x469200
};

// Scored controls ---------------------------------------------------------

// Control 45.
class HomingMissileControl : public ScoredControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x414830
  virtual void OnDestroy(Actor* actor) {}
  virtual int OnMove(Actor* actor);  // 0x414ac0
  virtual int GetScore(Actor* actor) { return 100; }
  virtual int GetHitPoints(Actor* actor) { return 2; }
};

// Control 48.
class MissileControl : public ScoredControl {
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
struct BossBodyWork {
  enum Flags {
    kDashing = 0x1,  // in PartWork::flags
    kTurning = 0x2,
  };

  char part_[0x20];    // 0x30, as PartWork (only its first 0x20 bytes)
  Actor* arms[4];      // 0x50, the parts on the left (0, 1) and right (2, 3)
  float deceleration;  // 0x60, of the dash
  short brake_time;    // 0x64, frames into the dash it starts to brake, 0:
                       // braking
  // A turn (RunTurnProgram): it speeds up until turn_peak, then slows down
  // until turn_end, turning by `turn` from `turn_from`.
  unsigned short turn_from;  // 0x66
  short turn;                // 0x68
  short turn_peak;           // 0x6a, frames into the turn
  short turn_end;            // 0x6c
};

// Control 32.
class BossBodyControl : public LinkedPartControl {
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
  int Dash(Actor* actor, EnmProgram* program, float x, float y, float power,
           int boost);  // 0x411e20
  // Run the movement programs of CEnmParam.
  void RunMoveProgram(Actor* actor, EnmProgram* program);            // 0x4120d0
  void RunTurnProgram(Actor* actor, EnmProgram* program);            // 0x412530
  void RunFireProgram(Actor* actor, EnmProgram* program, int side);  // 0x4119d0
  // Starts a turn of `actor` to `angle`; 0 when it faces it already.
  int StartTurn(Actor* actor, EnmProgram* program,
                short angle);  // 0x411580
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
  // Scratch space of RunFireProgram for ListWeapons.
  static int weapons_[7];  // 0x469064
};

// Control 33.
class BossArmControl : public LinkedPartControl {
 public:
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x412c30
};

// Control 34.
class JointPartControl : public LinkedPartControl {
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

// Control 40: bullets (spawns 13 bullets at a time).
class BulletCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(1000, 20, 0)
  virtual int Initialize();  // 0x413a40
  // OnCreate 0x413aa0, OnMove 0x413b00, Setup 0x4139c0

 private:
  static ActorDesc desc_;  // 0x469130
};

// Control 41: a fan of shells.
class ShellCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(1200, 30, 1)
  virtual int Initialize();  // 0x413e30
  // OnCreate 0x413ea0, OnMove 0x413f00, Setup 0x413dc0

 private:
  static ActorDesc desc_;  // 0x469164
};

// Control 42: a laser (control 43).
class LaserCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(1200, 30, 2)
  virtual int Initialize();  // 0x414040
  // OnCreate 0x4140d0, OnMove 0x414130, Setup 0x413fc0
};

// Control 44: homing missiles (control 45).
class HomingCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(2000, 40, 3)
  virtual int Initialize() { return 1; }
  // OnCreate 0x414700, OnMove 0x414760, Setup 0x414650
};

// Control 47: slow missiles (control 48).
class MissileCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(2000, 40, 5)
  virtual int Initialize() { return 1; }
  // OnCreate 0x413c60, OnMove 0x413cc0, Setup 0x413be0
};

// Control 46: a spray of sparks.
class SparkCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(2500, 40, 4)
  virtual int Initialize();  // 0x414ba0
  // OnCreate 0x414c10, OnMove 0x414c70, Setup 0x414b40

 private:
  static ActorDesc desc_;  // 0x469208
};

// Control 49: a sweeping stream of bullets.
class StreamCoreControl : public CorePartControl {
  WF_CORE_CONTROL_BODY(2500, 40, 6)
  virtual int Initialize();  // 0x414e20
  // OnCreate 0x414e80, OnMove 0x414f20, Setup 0x414dc0

 private:
  static ActorDesc desc_;  // 0x46923c
};

#undef WF_CORE_CONTROL_BODY

#endif  // WF_ACTOR_CONTROL_H_
