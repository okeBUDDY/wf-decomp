// ActorSystem: owns the actor pool, the draw lists and the ActorControl
// registered for each actor id (vtable 0x451d50, global instance
// g_actor_system, 0x4674c8).
#ifndef WF_ACTOR_SYSTEM_H_
#define WF_ACTOR_SYSTEM_H_

#include <windows.h>

#include <d3d.h>

#include "system.h"

class Actor;
class ActorControl;
class HLSObject;

// What ActorSystem::Spawn() makes an actor from.
struct ActorDesc {
  unsigned int flags;          // 0x00
  HLSObject* sprite;           // 0x04, the null sprite when NULL
  Actor* parent;               // 0x08
  D3DCOLOR color;              // 0x0c
  float unknown_10_;           // 0x10
  float unknown_14_;           // 0x14
  int work;                    // 0x18
  float x;                     // 0x1c
  float y;                     // 0x20
  unsigned short layer;        // 0x24
  unsigned short team;         // 0x26
  short hit_points;            // 0x28
  short debris;                // 0x2a
  unsigned short angle;        // 0x2c
  unsigned short start_angle;  // 0x2e
  unsigned short age;          // 0x30
};

class ActorSystem : public System {
 public:
  enum { kLayers = 16 };

  ActorSystem();                             // 0x408050
  virtual int Initialize(Application* app);  // 0x408970
  virtual void Finalize();                   // 0x408b70
  virtual void Pause() {}
  virtual void Resume() {}

  // Stores `control` under `id` (1..), replacing and deleting the previous
  // one. Returns 0 when `control` is NULL or refuses to initialize.
  int Register(int id, ActorControl* control);  // 0x407980
  // Creates an actor run by control `id` (a negative id may take any slot,
  // a positive one only those after the reserved ones) as a child of
  // `parent` at (`x`, `y`). Returns NULL when the pool is full.
  Actor* Create(int id, Actor* parent, float x, float y, unsigned short layer,
                unsigned short angle,
                void* param);  // 0x4080f0
  // Creates an actor without a control from `desc`.
  Actor* Spawn(const ActorDesc* desc);  // 0x408190
  // Moves every actor and frees the destroyed ones.
  void MoveAll();  // 0x408c20
  // Draws the layers in order.
  void DrawAll();  // 0x4081f0
  // Destroys every actor.
  void DestroyAll();  // 0x408480

  static ActorSystem* instance_;  // 0x468eb4
  static int flags_;              // 0x468eb8, 1: actors hit each other

 private:
  friend class Actor;

  // Puts `actor` on the draw list of its layer, next to an actor with the
  // same texture.
  void Link(Actor* actor);  // 0x407b60

  unsigned short max_controls_;  // 0x08
  unsigned short max_actors_;    // 0x0a
  unsigned short reserved_;      // 0x0c, slots only negative ids take
  Application* app_;             // 0x10
  short count_;                  // 0x14
  short last_dead_;              // 0x16, the highest slot destroyed this frame
  float left_;                   // 0x18, actors outside are destroyed
  float top_;                    // 0x1c
  float right_;                  // 0x20
  float bottom_;                 // 0x24
  ActorControl** controls_;      // 0x28, indexed by id
  Actor* actors_;                // 0x2c, new[]ed; 0 is the root
  Actor* layers_[kLayers];       // 0x30, the draw lists
};

extern ActorSystem g_actor_system;  // 0x4674c8

#endif  // WF_ACTOR_SYSTEM_H_
