// ActorSystem: owns the actors and the ActorControl registered for each
// actor id (global instance g_actor_system, 0x4674c8).
#ifndef WF_ACTOR_SYSTEM_H_
#define WF_ACTOR_SYSTEM_H_

#include "object.h"

class ActorControl;

class ActorSystem : public Object {
 public:
  // Stores `control` under `id` (1..), replacing and deleting the previous
  // one. Returns 0 when `control` is NULL or refuses to initialize.
  int Register(int id, ActorControl* control);  // 0x407980

 private:
  char unknown_08_[0x20];   // 0x08
  ActorControl** controls_;  // 0x28, indexed by id
};

extern ActorSystem g_actor_system;  // 0x4674c8

#endif  // WF_ACTOR_SYSTEM_H_
