// Actor: one object in the ActorSystem pool (0x120 bytes each, vtable
// 0x451d4c). The pool keeps the actors in place; an actor is in use while
// `flags` is non-zero. Its behaviour comes from the ActorControl registered
// under `id`. Actors form a tree (the children of an actor move with it)
// and each is on the draw list of its layer.
#ifndef WF_ACTOR_H_
#define WF_ACTOR_H_

#include <windows.h>

#include "hl_3d.h"
#include "hls_anm.h"

class ActorControl;
class ActorSystem;
struct ActorDesc;
class Channel;
class CSndBuf;

// A rectangle in screen coordinates.
struct FloatRect {
  float left;
  float top;
  float right;
  float bottom;
};

// A D3DCOLOR split into its bytes.
struct Color {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
};

// The sprite of an actor without one: draws nothing (vtable 0x451d3c).
class HLSNullObject : public HLSObject {
 public:
  virtual void Draw(const Transform* transform, D3DCOLOR color, int blend) {}
};

extern HLSNullObject g_null_sprite;  // 0x468ed8

class Actor {
 public:
  enum Flags {
    kFaceStart = 0x1,  // turns to start_angle every frame
    kStopDies = 0x2,   // destroyed when it stops
    kMirrorX = 0x4,
    kMirrorY = 0x8,
    kAdditive = 0x10,       // drawn added to the screen
    kCreating = 0x20000,    // cleared once it is linked
    kMoved = 0x40000,       // the transform changed this frame
    kNew = 0x70000,         // set while it is being created
    kChild = 0x80000,       // moves relative to its parent
    kDead = 0x100000,       // destroyed (the root always is)
    kDebris = 0x200000,     // debris of a destroyed actor
    kMovedOnce = 0x400000,  // has moved at least once
  };
  enum Team {
    kTeamMask = 0x7,
    kHits = 0x8,          // hits actors of other teams
    kHitByOthers = 0x10,  // hit by actors of other teams
    kHittable = 0x20,     // marks the hit image
    kDamageMask = 0xc0,
    kDamageSelf = 0x0,     // takes the damage
    kDamageArmor = 0x40,   // takes the damage too
    kDamageParent = 0x80,  // passes the damage to its parent
    kDamageGuard = 0xc0,   // takes no damage
    kDamageMessage = 0x100,
  };
  typedef void (*DamageHook)(Actor* source, Actor* actor, int amount);
  typedef void (*DestroyHook)(Actor* source, Actor* actor);

  Actor();             // 0x407ce0
  virtual ~Actor() {}  // 0x407a90, vector deleting dtor 0x408090
  // Makes this the root of the actor tree.
  void InitRoot();  // 0x407be0
  // Clears the state for a new actor.
  void Reset();  // 0x407c80
  // Starts the actor run by `control` as a child of `parent` (the root
  // when NULL) at (`x`, `y`). Returns 0 when the control refuses it.
  int Init(ActorControl* control, Actor* parent, float x, float y,
           unsigned short layer, unsigned short angle,
           void* param);  // 0x407d40
  // The actor of another team under the actor in the hit image, if any.
  Actor* HitTest();  // 0x407aa0
  // Flashes the actor and its children as hit (until the next draw).
  // Not inlined into Damage in the original.
  __declspec(noinline) void Flash();  // 0x407b30
  // Sets the actor up from `desc` (without a control).
  int Restore(const ActorDesc* desc);  // 0x407ef0
  // Runs the control and moves the actor and its children. Returns 0 when
  // the actor is to be destroyed.
  int Move();  // 0x4085c0
  // Takes `amount` damage from `source`; returns what the control says
  // for kDamageMessage, else 0.
  int Damage(int amount, Actor* source);  // 0x4084b0
  // Destroys the actor and its children; leaves the debris actor when
  // `debris` and tells the parent's control when `notify`.
  void Destroy(int debris, int notify);  // 0x4082b0
  // Draws the sprite (and marks the hit image); returns the next actor of
  // the draw list.
  Actor* Draw();  // 0x407f90
  // Moves the actor to the draw list of `new_layer`.
  void SetLayer(unsigned short new_layer);  // 0x4108c0
  // Replaces the sprite (the null sprite for NULL).
  void SetSprite(HLSObject* new_sprite) {
    sprite->Reset();
    if (new_sprite == NULL) {
      new_sprite = &g_null_sprite;
    }
    sprite = new_sprite;
  }
  // Sets the color to `num` / `den` of the way from `from` to `to`.
  void LerpColor(DWORD from, DWORD to, int num, int den);  // 0x407900

  static ActorSystem* system_;       // 0x468ebc
  static CSndBuf* hit_sound_;        // 0x468ec0
  static CSndBuf* guard_sound_;      // 0x468ec4
  static Channel* hit_channel_;      // 0x468ec8
  static Channel* guard_channel_;    // 0x468ecc
  static DestroyHook destroy_hook_;  // 0x468ed0
  static DamageHook damage_hook_;    // 0x468ed4

  unsigned int flags;           // 0x04, Flags, 0 when the slot is free
  int hit_points;               // 0x08, negative while flashing
  int low_hit_points;           // 0x0c, an eighth of the starting ones
  unsigned short team;          // 0x10, Team
  unsigned short age;           // 0x12, frames since creation
  unsigned short hit_age;       // 0x14, the age when first hit
  short debris;                 // 0x16, control of the debris actor
  unsigned short angle;         // 0x18
  unsigned short start_angle;   // 0x1a
  float speed;                  // 0x1c, along start_angle
  float acceleration;           // 0x20
  const FloatRect* bounds;      // 0x24, keeps it inside (or it dies outside
                                // the screen)
  HLSObject* sprite;            // 0x28
  D3DCOLOR color;               // 0x2c
  int work[0x10];               // 0x30, for the control
  ActorControl* control;        // 0x70
  unsigned short hit_id;        // 0x74
  short id;                     // 0x76, control id
  unsigned short layer;         // 0x78
  unsigned short matrix_angle;  // 0x7a, the angle of the rotation in transform
  float scale;                  // 0x7c
  Actor* parent;                // 0x80
  Actor* next_sibling;          // 0x84, the one created before
  Actor* prev_sibling;          // 0x88, the one created after
  Actor* first_child;           // 0x8c, the oldest
  Actor* last_child;            // 0x90, the newest
  Actor* draw_prev;             // 0x94
  Actor* draw_next;             // 0x98
  Transform* transform;         // 0x9c, &screen or &relative
  Transform screen;             // 0xa0
  Transform relative;           // 0xe0, to the parent
};

#endif  // WF_ACTOR_H_
