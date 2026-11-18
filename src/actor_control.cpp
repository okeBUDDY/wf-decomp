#include "actor_control.h"

#include "actor_system.h"

// 0x40fef0
ActorControl::~ActorControl() {}

// 0x40ff00
int RegisterActorControls() {
  if (!g_actor_system.Register(1, new Control01)) {
    return 0;
  }
  if (!g_actor_system.Register(8, new Control08)) {
    return 0;
  }
  if (!g_actor_system.Register(16, new Control16)) {
    return 0;
  }
  if (!g_actor_system.Register(17, new Control17)) {
    return 0;
  }
  if (!g_actor_system.Register(18, new Control18)) {
    return 0;
  }
  if (!g_actor_system.Register(19, new Control19)) {
    return 0;
  }
  if (!g_actor_system.Register(20, new Control20)) {
    return 0;
  }
  if (!g_actor_system.Register(21, new Control21)) {
    return 0;
  }
  if (!g_actor_system.Register(22, new Control22)) {
    return 0;
  }
  if (!g_actor_system.Register(23, new Control23)) {
    return 0;
  }
  if (!g_actor_system.Register(35, new Control35)) {
    return 0;
  }
  if (!g_actor_system.Register(43, new Control43)) {
    return 0;
  }
  if (!g_actor_system.Register(32, new Control32)) {
    return 0;
  }
  if (!g_actor_system.Register(33, new Control33)) {
    return 0;
  }
  if (!g_actor_system.Register(34, new Control34)) {
    return 0;
  }
  if (!g_actor_system.Register(40, new Control40)) {
    return 0;
  }
  if (!g_actor_system.Register(41, new Control41)) {
    return 0;
  }
  if (!g_actor_system.Register(42, new Control42)) {
    return 0;
  }
  if (!g_actor_system.Register(44, new Control44)) {
    return 0;
  }
  if (!g_actor_system.Register(47, new Control47)) {
    return 0;
  }
  if (!g_actor_system.Register(46, new Control46)) {
    return 0;
  }
  if (!g_actor_system.Register(49, new Control49)) {
    return 0;
  }
  if (!g_actor_system.Register(45, new Control45)) {
    return 0;
  }
  if (!g_actor_system.Register(48, new Control48)) {
    return 0;
  }
  return 1;
}
