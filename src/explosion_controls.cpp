// Explosions: the player's (control 16) and the debris of the boss
// (controls 17..20).
#include "actor_control.h"

#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "game_data.h"
#include "hl_3d.h"
#include "hls_anm.h"
#include "snd_sys.h"
#include "sounds.h"

// The ring of a shockwave (vtable 0x4527d0).
class ShockwaveSprite : public HLSObject {
 public:
  ShockwaveSprite() {}  // 0x410530
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x415120
};

// Explosions being created by other explosions; only the outermost one
// plays its sound.
int g_explosion_depth;               // 0x469048
ActorDesc g_fragment_desc;           // 0x469014
ActorDesc g_spark_desc;              // 0x468fe0
ActorDesc g_shockwave_desc;          // 0x468fac
extern ShockwaveSprite g_shockwave;  // 0x46904c
extern EffectDesc g_smoke_effect;    // 0x45d3ac
extern AnmData g_explosion_anim;     // 0x45d37c

// 0x410540
// TODO: nonmatching (store order of the desc fields; depth counter load
// scheduled differently)
int Control16::Initialize() {
  memset(&g_fragment_desc, 0, sizeof(g_fragment_desc));
  memset(&g_spark_desc, 0, sizeof(g_spark_desc));
  g_fragment_desc.color = 0xffffffff;
  g_fragment_desc.team = 0;
  g_fragment_desc.age = 36;
  g_fragment_desc.flags = Actor::kStopDies;
  g_fragment_desc.hit_points = 1;
  g_spark_desc.color = 0xffffffff;
  g_spark_desc.team = 0;
  g_spark_desc.flags = Actor::kFaceStart;
  g_spark_desc.hit_points = 1;
  g_spark_desc.sprite = g_sys_pats.Get(8);
  memset(&g_shockwave_desc, 0, sizeof(g_shockwave_desc));
  g_spark_desc.layer = 0;
  g_spark_desc.speed = 8.0f;
  g_spark_desc.acceleration = 2.0f;
  g_shockwave_desc.team = 0;
  g_shockwave_desc.flags = Actor::kAdditive;
  g_shockwave_desc.hit_points = 1;
  g_shockwave_desc.color = 0xff808080;
  g_shockwave_desc.sprite = &g_shockwave;
  g_smoke_effect.sprite = g_add_pats.Get(1);
  return 1;
}

// 0x410640
// TODO: nonmatching (global depth counter load scheduled differently)
int Control16::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  ++g_explosion_depth;
  g_actor_system.Create(18, NULL, actor->transform->m[2][0],
                        actor->transform->m[2][1], 8, 0, NULL);
  --g_explosion_depth;
  g_spark_desc.x = actor->transform->m[2][0];
  g_spark_desc.y = actor->transform->m[2][1];
  for (int i = 0; i < 48 * 0x400; i += 0x400) {
    g_spark_desc.start_angle = i / 48;
    g_actor_system.Spawn(&g_spark_desc);
  }
  if (g_explosion_depth == 0) {
    g_application.sound->channel(6)->Assign(&g_snd_bomb_me, 0, 1.0f);
  }
  return 1;
}

// 0x410930
int Control17::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetSprite(g_anm_sys.Play(&g_explosion_anim));
  actor->start_angle = 0x200;
  actor->speed = 8.0f;
  actor->SetLayer(9);
  int count = g_actor_system.count() >= 700 ? 2 : 8;
  for (; count > 0; --count) {
    g_actor_system.Create(8, NULL, actor->transform->m[2][0],
                          actor->transform->m[2][1], 9, 0, &g_smoke_effect);
  }
  if (g_explosion_depth == 0) {
    g_application.sound->channel(0)->Assign(&g_snd_bomb1, 0, 1.0f);
  }
  return 1;
}

// 0x410a00
// TODO: nonmatching (global depth counter load scheduled differently)
int Control18::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetLayer(8);
  Transform* transform = actor->transform;
  ++g_explosion_depth;
  g_actor_system.Create(17, NULL, transform->m[2][0], transform->m[2][1], 9, 0,
                        NULL);
  if (--g_explosion_depth == 0) {
    g_application.sound->channel(1)->Assign(&g_snd_bomb2, 0, 1.0f);
  }
  return 1;
}

// Actor::work of control 19.
struct Control19Work {
  Actor* pieces[3];  // 0x30
  short spins[3];    // 0x3c
  short alpha;       // 0x42
};

// 0x410710
// TODO: nonmatching (global depth counter load scheduled differently)
int Control19::OnMove(Actor* actor) {
  Control19Work* work = reinterpret_cast<Control19Work*>(actor->work);
  if (work->alpha > 0) {
    work->alpha -= 3;
  }
  int alpha = work->alpha;
  D3DCOLOR color = ((((alpha | 0xffffff00) << 8) | alpha) << 8) | alpha;
  work->pieces[0]->angle += work->spins[0];
  work->pieces[0]->color = color;
  work->pieces[1]->angle += work->spins[1];
  work->pieces[1]->color = color;
  work->pieces[2]->angle += work->spins[2];
  work->pieces[2]->color = color;
  switch (actor->age) {
    case 4:
    case 8:
    case 12: {
      ++g_explosion_depth;
      float x = actor->transform->m[2][0];
      float y = actor->transform->m[2][1];
      g_actor_system.Create(18, NULL, x + ((rand() & 0x7f) - 64),
                            y + ((rand() & 0x7f) - 64), 8, 0, NULL);
      --g_explosion_depth;
      break;
    }
    case 36:
      return 0;
  }
  return 1;
}
