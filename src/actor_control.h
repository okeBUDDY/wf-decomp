// ActorControl: the behaviour of one kind of actor. One stateless instance
// of each subclass is registered in g_actor_system under its id by
// RegisterActorControls(); an actor calls the control of its id.
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
struct EnmSlot;

// Actor::work of the parts of the boss.
struct PartWork {
  EnmSlot* slot;      // 0x30
  int unknown_04_;    // 0x34
  void* param;        // 0x38
  int unknown_0c_;    // 0x3c
  short level;        // 0x40
  short unknown_12_;  // 0x42
};

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
};

class PartControl : public ScoredControl {
 public:
  virtual void OnDamage(Actor* actor, int how);  // 0x4198e0
  // Scales the hit points by the level of the part.
  void ScaleHitPoints(Actor* actor);  // 0x418d70
};

class LinkedPartControl : public PartControl {
 public:
  virtual void OnDestroy(Actor* actor);                        // 0x4195f0
  virtual int OnMove(Actor* actor);                            // 0x419cc0
  virtual void OnEvent(Actor* actor, int x, int y);            // 0x419370
  virtual void OnChildDestroyed(Actor* parent, Actor* child);  // 0x419400
  // 0x40fe30
  virtual int GetScore(Actor* actor);
  // 0x40fe40
  virtual int GetHitPoints(Actor* actor);
  // Spawns the glow (control 22) on `actor`; 0 when it cannot.
  int SpawnGlow(Actor* actor);  // 0x418ec0
};

class CorePartControl : public PartControl {
 public:
  virtual void OnDestroy(Actor* actor);              // 0x4197d0
  virtual void OnEvent(Actor* actor, int x, int y);  // 0x418cc0
  virtual int GetLevel() = 0;
  virtual void Setup(Actor* actor, int rank) = 0;
};

// Simple controls ---------------------------------------------------------

class Control01 : public ActorControl {
 public:
  virtual int Initialize();  // 0x412fe0
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);   // 0x413550
  virtual void OnDestroy(Actor* actor);              // 0x412fa0
  virtual int OnMove(Actor* actor);                  // 0x4136f0
  virtual void OnEvent(Actor* actor, int x, int y);  // 0x4130f0
};

class Control08 : public ActorControl {
 public:
  virtual int Initialize();  // 0x412c80
  virtual void Finalize() {}
  virtual int OnCreate(Actor* actor, void* param);  // 0x412cb0
  virtual void OnDestroy(Actor* actor) {}
  virtual int OnMove(Actor* actor);  // 0x412e70
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

class Control32 : public PartControl {
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

// Core part controls: score, hit points and level are constants.
#define WF_CORE_CONTROL(name, score, hit_points, level)           \
  class name : public CorePartControl {                           \
   public:                                                        \
    virtual int Initialize();                                     \
    virtual void Finalize() {}                                    \
    virtual int OnCreate(Actor* actor, void* param);              \
    virtual int OnMove(Actor* actor);                             \
    virtual int GetScore(Actor* actor) { return score; }          \
    virtual int GetHitPoints(Actor* actor) { return hit_points; } \
    virtual int GetLevel() { return level; }                      \
    virtual void Setup(Actor* actor, int rank);                   \
  }

// Controls 44 and 47 keep Initialize() { return 1; }.
#define WF_CORE_CONTROL_SIMPLE(name, score, hit_points, level)    \
  class name : public CorePartControl {                           \
   public:                                                        \
    virtual int Initialize() { return 1; }                        \
    virtual void Finalize() {}                                    \
    virtual int OnCreate(Actor* actor, void* param);              \
    virtual int OnMove(Actor* actor);                             \
    virtual int GetScore(Actor* actor) { return score; }          \
    virtual int GetHitPoints(Actor* actor) { return hit_points; } \
    virtual int GetLevel() { return level; }                      \
    virtual void Setup(Actor* actor, int rank);                   \
  }

WF_CORE_CONTROL(Control40, 1000, 20, 0);
WF_CORE_CONTROL(Control41, 1200, 30, 1);
WF_CORE_CONTROL(Control42, 1200, 30, 2);
WF_CORE_CONTROL_SIMPLE(Control44, 2000, 40, 3);
WF_CORE_CONTROL_SIMPLE(Control47, 2000, 40, 5);
WF_CORE_CONTROL(Control46, 2500, 40, 4);
WF_CORE_CONTROL(Control49, 2500, 40, 6);

#undef WF_CORE_CONTROL
#undef WF_CORE_CONTROL_SIMPLE

// Registers one instance of every control in g_actor_system; 0 on failure.
int RegisterActorControls();  // 0x40ff00

#endif  // WF_ACTOR_CONTROL_H_
