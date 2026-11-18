// Explosions: the player's (control 16) and the debris of the boss
// (controls 17..20).
#include "actor_control.h"

#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "game.h"
#include "game_data.h"
#include "hl_3d.h"
#include "hls_anm.h"
#include "line_sprites.h"
#include "snd_sys.h"
#include "sounds.h"
#include "sprite.h"

// Explosions being created by other explosions; only the outermost one
// plays its sound.
int g_explosion_depth;             // 0x469048
ActorDesc g_fragment_desc;         // 0x469014
ActorDesc g_spark_desc;            // 0x468fe0
ActorDesc g_shockwave_desc;        // 0x468fac
extern EffectDesc g_smoke_effect;  // 0x45d3ac
extern AnmData g_explosion_anim;   // 0x45d37c
extern AnmData g_fragment_anim;    // 0x45d3a4

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
    g_application.sound->channel(6)->Assign(&g_sounds[kSndBombMe], 0, 1.0f);
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
    g_application.sound->channel(0)->Assign(&g_sounds[kSndBomb1], 0, 1.0f);
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
    g_application.sound->channel(1)->Assign(&g_sounds[kSndBomb2], 0, 1.0f);
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
      float y = actor->transform->m[2][1];
      float x = actor->transform->m[2][0];
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

// 0x410a80
int Control18::OnMove(Actor* actor) {
  switch (actor->age) {
    case 4:
    case 8:
    case 12: {
      ++g_explosion_depth;
      float y = actor->transform->m[2][1];
      float x = actor->transform->m[2][0];
      g_actor_system.Create(17, NULL, x + ((rand() & 0x7f) - 64),
                            y + ((rand() & 0x7f) - 64), 9, 0, NULL);
      --g_explosion_depth;
      break;
    }
    case 13:
      return 0;
  }
  if (actor->age < 6) {
    g_fragment_desc.x = actor->transform->m[2][0];
    g_fragment_desc.y = actor->transform->m[2][1];
    g_fragment_desc.layer = 8;
    g_fragment_desc.acceleration = -0.1f;
    for (int count = g_actor_system.count() >= 700 ? 1 : 4; count > 0;
         --count) {
      g_fragment_desc.start_angle = rand() & 0x3ff;
      g_fragment_desc.sprite = g_anm_sys.Play(&g_fragment_anim);
      g_fragment_desc.speed = static_cast<float>((rand() & 7) + 1);
      g_actor_system.Spawn(&g_fragment_desc);
    }
  }
  return 1;
}

// 0x410be0
// TODO: nonmatching (the original sinks the depth counter store below the
// pushes)
int Control19::OnCreate(Actor* actor, void* param) {
  Control19Work* work = reinterpret_cast<Control19Work*>(actor->work);
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetLayer(8);
  g_spark_desc.x = actor->transform->m[2][0];
  g_spark_desc.y = actor->transform->m[2][1];
  if (g_actor_system.count() < 700) {
    for (int i = 0; i < 48; ++i) {
      g_spark_desc.start_angle = i * 0x400 / 48;
      g_actor_system.Spawn(&g_spark_desc);
    }
  }
  g_shockwave_desc.layer = 7;
  g_shockwave_desc.parent = actor;
  for (int i = 0; i < 3; ++i) {
    g_shockwave_desc.angle = (rand() & 0x7f) + i * 300;
    work->pieces[i] = g_actor_system.Spawn(&g_shockwave_desc);
    if (rand() & 1) {
      work->spins[i] = (rand() & 7) + 2;
    } else {
      work->spins[i] = -2 - (rand() & 7);
    }
  }
  work->alpha = 0x7e;
  ++g_explosion_depth;
  g_actor_system.Create(18, NULL, actor->transform->m[2][0],
                        actor->transform->m[2][1], 8, 0, NULL);
  if (--g_explosion_depth == 0) {
    g_application.sound->channel(2)->Assign(&g_sounds[kSndBomb3], 0, 1.0f);
  }
  return 1;
}

// Actor::work of control 20.
struct Control20Work {
  Actor* core;      // 0x30
  Actor* rings[3];  // 0x34
  short spins[3];   // 0x40
  short alpha;      // 0x46
};

// 0x410d50
int Control20::OnCreate(Actor* actor, void* param) {
  Control20Work* work = reinterpret_cast<Control20Work*>(actor->work);
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetSprite(g_sys_pats.Get(32));
  actor->angle = (rand() & 0x3f) - 0x20;
  actor->start_angle = (rand() & 0x7f) - 0x40;
  actor->speed = 2.0f;
  actor->acceleration = -0.02f;
  actor->SetLayer(4);
  ActorDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.flags = 0;
  desc.team = 0;
  desc.color = 0xffc06000;
  desc.hit_points = 1;
  HLSObject* core = g_enemy_pats.Get(1);
  desc.sprite = core;
  desc.parent = actor;
  desc.x = 0.0f;
  desc.y = 0.0f;
  desc.layer = 4;
  work->core = g_actor_system.Spawn(&desc);
  if (g_explosion_depth == 0) {
    g_application.sound->channel(2)->Assign(&g_sounds[kSndBomb3], 0, 1.0f);
  }
  g_boss = actor;
  return 1;
}

// 0x410e70
// TODO: nonmatching (case 150: the original sinks the stores of the sprite
// and the depth counter below the loads of the position)
int Control20::OnMove(Actor* actor) {
  Control20Work* work = reinterpret_cast<Control20Work*>(actor->work);
  switch (actor->age) {
    case 50:
    case 80:
      actor->angle = (rand() & 0x7f) - 64;
      actor->start_angle = (rand() & 0x1ff) + 0x100;
      actor->speed = 2.0f;
      actor->acceleration = -0.02f;
      g_actor_system.Create(18, NULL, actor->transform->m[2][0],
                            actor->transform->m[2][1], 8, 0, NULL);
      break;
    case 90:
      g_shockwave_desc.layer = 7;
      g_shockwave_desc.parent = actor;
      for (int i = 0; i < 3; ++i) {
        g_shockwave_desc.angle = (rand() & 0x7f) + i * 300;
        work->rings[i] = g_actor_system.Spawn(&g_shockwave_desc);
        if (rand() & 1) {
          work->spins[i] = (rand() & 7) + 1;
        } else {
          work->spins[i] = -1 - (rand() & 7);
        }
        work->rings[i]->color = 0;
      }
      work->alpha = 0;
      break;
    case 100:
      g_application.sound->channel(4)->Assign(&g_sounds[kSndBombPre], 0, 1.0f);
      break;
    case 113:
      g_global.screen_fade->SetColor(0xff, 0xff, 0xff, 0);
      break;
    case 110:
    case 120:
      g_global.screen_fade->SetColor(0xff, 0xff, 0xff, 0x40);
      break;
    case 123:
      g_global.screen_fade->SetColor(0xff, 0xff, 0xff, 0);
      g_global.screen_fade->FadeTo(28, 0xff, 0xff, 0xff, 0x40);
      break;
    case 150:
      for (int i = 0; i < 3; ++i) {
        work->rings[i]->Destroy(1, 1);
      }
      work->core->Destroy(1, 1);
      actor->SetSprite(NULL);
      ++g_explosion_depth;
      g_actor_system.Create(19, NULL, actor->transform->m[2][0],
                            actor->transform->m[2][1], 8, 0, NULL);
      --g_explosion_depth;
      g_global.screen_fade->SetColor(0xff, 0xff, 0xff, 0xff);
      g_global.screen_fade->FadeTo(120, 0xff, 0xff, 0xff, 0);
      g_application.sound->channel(4)->Stop();
      g_application.sound->channel(3)->Assign(&g_sounds[kSndBomb4], 0, 1.0f);
      g_application.sound->channel(5)->Assign(&g_sounds[kSndBombAll], 0, 1.0f);
      break;
    case 165:
    case 180: {
      ++g_explosion_depth;
      float y = actor->transform->m[2][1];
      float x = actor->transform->m[2][0];
      g_actor_system.Create(19, NULL, x + ((rand() & 0x7f) - 64),
                            y + ((rand() & 0x7f) - 64), 8, 0, NULL);
      --g_explosion_depth;
      break;
    }
    case 270:
      return 0;
  }
  if (actor->age < 150) {
    work->core->color =
        (g_application.frame_count & 2) ? 0xffff4000 : 0xff00ff00;
    if ((rand() & 7) == 0) {
      float y = actor->transform->m[2][1];
      float x = actor->transform->m[2][0];
      g_actor_system.Create(17, NULL, x + ((rand() & 0x3f) - 31),
                            y + ((rand() & 0x7f) - 64), 9, 0, NULL);
    }
    if (actor->age > 90) {
      work->alpha += 2;
      int alpha = work->alpha;
      D3DCOLOR color = ((((alpha | 0xffffff00) << 8) | alpha) << 8) | alpha;
      work->rings[0]->angle += work->spins[0];
      work->rings[0]->color = color;
      work->rings[1]->angle += work->spins[1];
      work->rings[1]->color = color;
      work->rings[2]->angle += work->spins[2];
      work->rings[2]->color = color;
    }
  }
  return 1;
}
