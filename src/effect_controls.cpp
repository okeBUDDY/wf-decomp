// Effect sprites: control 8 runs an EffectDesc, control 23 is the flare of
// a part (PartControl::SpawnFlare) that leaves a trail of them.
#include "actor_control.h"

#include <stdlib.h>

#include "actor.h"
#include "actor_system.h"
#include "enm_param.h"
#include "game_data.h"

// Actor::work of control 8.
struct EffectWork {
  int fading;      // 0x30, the color changes
  int life;        // 0x34
  float grow;      // 0x38
  short color[4];  // 0x3c, red, green, blue, alpha
  short fade[4];   // 0x44
};

// Adds the fade of color channel `i` to it, keeping it within 0..255.
#define WF_FADE_CHANNEL(work, i)         \
  if ((work)->fade[i] != 0) {            \
    (work)->color[i] += (work)->fade[i]; \
    if ((work)->color[i] < 0) {          \
      (work)->color[i] = 0;              \
    }                                    \
    if ((work)->color[i] > 255) {        \
      (work)->color[i] = 255;            \
    }                                    \
  }

// 0x412c80
int Control08::Initialize() {
  HLSObject* sprite = g_pats.add[0].Get(1);
  default_.sprite = sprite;
  return 1;
}

// 0x412cb0
int Control08::OnCreate(Actor* actor, void* param) {
  EffectWork* work = reinterpret_cast<EffectWork*>(actor->work);
  const EffectDesc* desc = static_cast<const EffectDesc*>(param);
  if (desc == NULL) {
    desc = &default_;
  }
  actor->hit_points = 1;
  actor->flags |= desc->flags;
  actor->team = 0;
  actor->SetSprite(desc->sprite);
  actor->acceleration = desc->acceleration;
  if (desc->angle_min == desc->angle_max) {
    actor->start_angle = desc->angle_min;
  } else {
    actor->start_angle =
        desc->angle_min + rand() % (desc->angle_max - desc->angle_min);
  }
  if (desc->speed_min == desc->speed_max) {
    actor->speed = desc->speed_min;
  } else {
    actor->speed = (static_cast<float>(rand() & 0x3ff) / 1024.0f) *
                       (desc->speed_max - desc->speed_min) +
                   desc->speed_min;
  }
  if (desc->scale_min == desc->scale_max) {
    actor->scale = desc->scale_min;
  } else {
    actor->scale = (static_cast<float>(rand() & 0x3ff) / 1024.0f) *
                       (desc->scale_max - desc->scale_min) +
                   desc->scale_min;
  }
  actor->matrix_angle += 0x1000;
  actor->color =
      (((((desc->color[3] << 8) | desc->color[0]) << 8) | desc->color[1])
       << 8) |
      desc->color[2];
  if (desc->fade[1] | desc->fade[0] | desc->fade[2] | desc->fade[3]) {
    work->fading = 1;
    work->color[0] = desc->color[0];
    work->color[1] = desc->color[1];
    work->color[2] = desc->color[2];
    work->color[3] = desc->color[3];
    work->fade[0] = desc->fade[0];
    work->fade[1] = desc->fade[1];
    work->fade[2] = desc->fade[2];
    work->fade[3] = desc->fade[3];
  } else {
    work->fading = 0;
  }
  if (desc->life == 0 && desc->grow >= 0.0f && desc->fade[3] >= 0) {
    actor->flags |= Actor::kStopDies;
  }
  work->life = desc->life;
  work->grow = desc->grow;
  return 1;
}

// 0x412e70
int Control08::OnMove(Actor* actor) {
  EffectWork* work = reinterpret_cast<EffectWork*>(actor->work);
  if (work->life != 0 && actor->age > work->life) {
    return 0;
  }
  if (work->grow != 0.0f) {
    actor->SetScale(actor->scale + work->grow);
    if (actor->scale < 0.0f) {
      return 0;
    }
  }
  if (work->fading) {
    WF_FADE_CHANNEL(work, 0);
    WF_FADE_CHANNEL(work, 1);
    WF_FADE_CHANNEL(work, 2);
    WF_FADE_CHANNEL(work, 3);
    if (work->color[3] == 0) {
      return 0;
    }
    actor->color =
        (((((work->color[3] << 8) | work->color[0]) << 8) | work->color[1])
         << 8) |
        work->color[2];
  }
  return 1;
}

// Actor::work of control 23.
struct FlareWork {
  float max_scale;  // 0x30
  float scale;      // 0x34
  int timer;        // 0x38, frames left
};

// The trail of control 23.
extern EffectDesc g_flare_trail;  // 0x45d3e0

// 0x411380
// TODO: nonmatching (the original loads start_angle into dx and stores
// hit_points first).
int Control23::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->angle = actor->start_angle;
  actor->team = 0;
  FlareWork* work = reinterpret_cast<FlareWork*>(actor->work);
  work->timer = g_enm_param.flare_time_;
  work->max_scale = ((g_enm_param.flare_size_ - 1.0f) * 1.5f + 1.0f) * 0.2f;
  work->scale = 0.0f;
  return 1;
}

// 0x411450
int Control23::OnMove(Actor* actor) {
  FlareWork* work = reinterpret_cast<FlareWork*>(actor->work);
  if (--work->timer <= 0) {
    return 0;
  }
  work->scale += 0.04f;
  if (work->scale > work->max_scale) {
    work->scale = work->max_scale;
  }
  if ((actor->age & 3) == 1) {
    HLSObject* sprite = g_pats.add[0].Get(28);
    g_flare_trail.sprite = sprite;
    g_flare_trail.scale_min = g_flare_trail.scale_max = work->scale;
    g_flare_trail.angle_min = g_flare_trail.angle_max = actor->ScreenAngle();
    g_actor_system.Create(8, NULL, actor->screen.m[2][0], actor->screen.m[2][1],
                          actor->layer, 0, &g_flare_trail);
  }
  return 1;
}
