#include "actor_control.h"

#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "game.h"
#include "game_data.h"

// Actor::work of control 22: fades in, glows for a while and fades out.
struct GlowWork {
  int timer;     // frames in the state
  int state;     // 0 start, 1 fade in, 2 glow, 3 fade out
  int duration;  // of the glow
  int phase;     // of the glow's pulse
  short spin;    // added to the angle every frame
};

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

// 0x410350
int Control22::OnMove(Actor* actor) {
  GlowWork* work = reinterpret_cast<GlowWork*>(actor->work);
  float alpha;
  switch (work->state) {
    case 0:
      work->phase = rand() & 0xff;
      work->spin = (rand() & 1) ? 2 : -2;
      work->timer = 0;
      ++work->state;
    case 1:
      alpha = work->timer * (1.0f / 60.0f);
      if (work->timer >= 60) {
        work->timer = 0;
        ++work->state;
      }
      break;
    case 2:
      alpha = 1.0f;
      if (work->timer >= work->duration) {
        work->timer = 0;
        work->state = 3;
      }
      break;
    case 3:
      alpha = 1.0f - work->timer * (1.0f / 60.0f);
      if (work->timer >= 60) {
        return 1;
      }
      break;
  }
  actor->color =
      (static_cast<int>((g_sin_table[(work->phase & 0xff) * 4] * 0.2f + 0.8f) *
                        alpha * 255.0f)
       << 24) |
      0xff8800;
  actor->angle += work->spin;
  ++work->phase;
  ++work->timer;
  return 1;
}

// 0x410470
int Control22::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetSprite(g_add_pats.Get(27));
  actor->flags |= Actor::kAdditive;
  GlowWork* work = reinterpret_cast<GlowWork*>(actor->work);
  work->duration = reinterpret_cast<int>(param) - 120;
  work->state = 0;
  return 1;
}

// 0x4108c0
void Actor::SetLayer(unsigned short new_layer) {
  if (flags & kCreating) {
    layer = new_layer;
    return;
  }
  if (draw_prev != NULL) {
    draw_prev->draw_next = draw_next;
  } else {
    system_->layers_[layer] = draw_next;
  }
  if (draw_next != NULL) {
    draw_next->draw_prev = draw_prev;
  }
  layer = new_layer;
  system_->Link(this);
}

// 0x4104d0
void Control16::OnDestroy(Actor* actor) { g_game.SpawnPlayer(); }

// 0x4104e0
int Control16::OnMove(Actor* actor) { return actor->age <= 60; }

// 0x410500
int Control17::OnMove(Actor* actor) { return actor->age <= 18; }

// 0x410520
void Control20::OnDestroy(Actor* actor) { g_boss = NULL; }

// 0x4127c0
void Control35::OnDestroy(Actor* actor) { --count_; }

// 0x412820
int Control35::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_sys_pats.Get(64);
  desc_.sprite = sprite;
  desc_.flags = Actor::kFaceStart;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 6.0f;
  desc_.layer = 13;
  return 1;
}

// 0x412fa0
void Control01::OnDestroy(Actor* actor) { g_game.OnPlayerDestroyed(); }

// 0x413c30
int Control48::OnMove(Actor* actor) {
  if (actor->speed <= 0.0f) {
    return 0;
  }
  return 1;
}
