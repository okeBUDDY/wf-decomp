// The weapons of the cores (controls 40..49) and what they fire.
#include "actor_control.h"

#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "enm_param.h"
#include "game.h"
#include "game_data.h"
#include "hl_3d.h"
#include "hls_anm.h"
#include "snd_sys.h"
#include "sounds.h"

// 0x413c30
int Control48::OnMove(Actor* actor) {
  if (actor->speed <= 0.0f) {
    return 0;
  }
  return 1;
}

struct Control42Work;

// Actor::work of control 43, the laser of control 42.
struct Control43Work {
  Control42Work* owner;  // 0x30
  float scale;           // 0x34, of the glow
  Actor* glow;           // 0x38, at the muzzle
  Actor* head;           // 0x3c, the first segment of the beam
  short timer;           // 0x40, frames until it ends (0: none)
  short sound;           // 0x42, 0/1: holds channel 19/20, 2: fading out
};

// 0x414430
void Control43::OnDestroy(Actor* actor) {
  switch (reinterpret_cast<Control43Work*>(actor->work)->sound) {
    case 0:
      if (--sound_users_[0] == 0) {
        g_application.sound->channel(19)->Stop();
      }
      break;
    case 1:
      if (--sound_users_[1] == 0) {
        g_application.sound->channel(20)->Stop();
      }
      break;
  }
}

// Core controls -------------------------------------------------------------

// Actor::work of control 40.
struct Control40Work : PartWork {
  short interval;             // 0x54, frames of firing
  unsigned short fire_angle;  // 0x56
  short delay;                // 0x58, until the next round
                              // 0x5a: padding
  float speed;                // 0x5c, of the bullets
};

// 0x4139c0
void Control40::Setup(Actor* actor, int rank) {
  Control40Work* work = reinterpret_cast<Control40Work*>(actor->work);
  short interval = rank / 10 * 4 + 4;
  work->interval = interval;
  work->speed = 4.0f;
  if (rank > 50) {
    work->speed = (rank - 50) * 0.0625f + 4.0f;
  }
  work->delay = 50 - rank * 30 / 100;
  work->volley_time = interval;
}

// 0x413a40
int Control40::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.sys[0].Get(64);
  desc_.sprite = sprite;
  desc_.flags = Actor::kFaceStart;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 6.0f;
  desc_.layer = 13;
  return 1;
}

// 0x413aa0
int Control40::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(48));
  actor->debris = 17;
  OnDamage(actor, kEventAim);
  return 1;
}

// 0x413b00
int Control40::OnMove(Actor* actor) {
  Control40Work* work = reinterpret_cast<Control40Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  short step = work->step;
  switch (step) {
    case 0:
      work->fire_angle = actor->ScreenAngle();
      work->timer = work->interval;
      work->step = ++step;
      // Fall through.
    case 1:
      if (!(work->timer & 3)) {
        desc_.x = actor->screen.m[2][0];
        desc_.y = actor->screen.m[2][1];
        desc_.start_angle = work->fire_angle;
        desc_.speed = work->speed;
        desc_.work = reinterpret_cast<int>(work->slot);
        g_actor_system.Spawn(&desc_);
        g_application.sound->channel(17)->Assign(&g_sounds[kSndFireBl], 0,
                                                 1.0f);
      }
      if (--work->timer <= 0) {
        Schedule(actor, work->delay, kEventAim);
      }
      break;
  }
  return 1;
}

// Actor::work of control 47.
struct Control47Work : PartWork {
  float speed;  // 0x54, of the missiles
  short delay;  // 0x58
};

// 0x413be0
void Control47::Setup(Actor* actor, int rank) {
  Control47Work* work = reinterpret_cast<Control47Work*>(actor->work);
  work->speed = rank * 0.05f + 5.0f;
  work->delay = 240 - rank * 150 / 100;
}

// 0x413c60
int Control47::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(53));
  actor->debris = 17;
  OnDamage(actor, kEventAim);
  return 1;
}

// 0x413cc0
int Control47::OnMove(Actor* actor) {
  Control47Work* work = reinterpret_cast<Control47Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  g_actor_system.Create(48, NULL, actor->screen.m[2][0], actor->screen.m[2][1],
                        11, actor->ScreenAngle(), work);
  g_application.sound->channel(22)->Assign(&g_sounds[kSndFireMs], 0, 1.0f);
  Schedule(actor, work->delay, kEventAim);
  return 1;
}

// Actor::work of control 48.
struct Control48Work {
  int unused[2];            // 0x30, not set: a missile carries no EnmSlot
                            // in work[0], unlike the bullets
  Control47Work* launcher;  // 0x38
};

// 0x413d50
int Control48::OnCreate(Actor* actor, void* param) {
  Control48Work* work = reinterpret_cast<Control48Work*>(actor->work);
  work->launcher = static_cast<Control47Work*>(param);
  actor->team = 0x61;
  actor->debris = 19;
  actor->SetSprite(g_pats.sys[0].Get(67));
  actor->angle = actor->start_angle;
  actor->acceleration = -0.05f;
  actor->speed = work->launcher->speed;
  return 1;
}

// Actor::work of control 41.
struct Control41Work : PartWork {
  short interval;  // 0x54
  short count;     // 0x56, shells in the fan
  short spread;    // 0x58, angle between two shells
                   // 0x5a: padding
  float speed;  // 0x5c
};

// 0x413dc0
void Control41::Setup(Actor* actor, int rank) {
  Control41Work* work = reinterpret_cast<Control41Work*>(actor->work);
  work->count = (rank >> 3) + 1;
  work->spread = 0x40;
  if (work->count * 0x40 > 0x400) {
    work->spread = 0x400 / work->count;
  }
  work->speed = rank * 0.025f + 3.0f;
  work->interval = 40 - rank * 20 / 100;
}

// 0x413e30
int Control41::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.sys[0].Get(65);
  desc_.sprite = sprite;
  desc_.flags = 0;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 4.0f;
  desc_.layer = 13;
  desc_.debris = 0;
  return 1;
}

// 0x413ea0
int Control41::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(49));
  actor->debris = 17;
  OnDamage(actor, kEventAim);
  return 1;
}

// 0x413f00
int Control41::OnMove(Actor* actor) {
  Control41Work* work = reinterpret_cast<Control41Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  desc_.x = actor->screen.m[2][0];
  desc_.y = actor->screen.m[2][1];
  desc_.speed = work->speed;
  desc_.work = reinterpret_cast<int>(work->slot);
  int count = work->count;
  int angle = static_cast<short>(actor->ScreenAngle()) -
              ((count - 1) * work->spread >> 1);
  for (; count > 0; --count) {
    desc_.start_angle = angle;
    g_actor_system.Spawn(&desc_);
    angle += work->spread;
  }
  g_application.sound->channel(18)->Assign(&g_sounds[kSndFireCn], 0, 1.0f);
  Schedule(actor, work->interval, kEventAim);
  return 1;
}

// Actor::work of control 42.
struct Control42Work : PartWork {
  Actor* laser;    // 0x54, control 43 while it fires
  short mode;      // 0x58, 0..2: the width of the beam
  short duration;  // 0x5a
  short delay;     // 0x5c
};

// 0x413fc0
void Control42::Setup(Actor* actor, int rank) {
  Control42Work* work = reinterpret_cast<Control42Work*>(actor->work);
  if (rank < 33) {
    work->mode = 0;
  } else {
    work->mode = (rank >= 66) + 1;
  }
  work->duration = (rank / 10 + 3) * 20;
  work->laser = NULL;
  work->delay = 180 - rank * 90 / 100;
  work->volley_time = work->duration + 10;
}

// 0x414040
int Control42::Initialize() {
  memset(&Control43::beam_desc_, 0, sizeof(Control43::beam_desc_));
  Control43::beam_desc_.color = 0xffffffff;
  Control43::beam_desc_.flags = Actor::kFaceStart | Actor::kAdditive;
  Control43::beam_desc_.team = 0x21;
  Control43::beam_desc_.hit_points = 1;
  Control43::beam_desc_.layer = 12;
  memset(&Control43::glow_desc_, 0, sizeof(Control43::glow_desc_));
  Control43::glow_desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.add[0].Get(16);
  Control43::glow_desc_.sprite = sprite;
  Control43::glow_desc_.flags = Actor::kAdditive;
  Control43::glow_desc_.team = 0;
  Control43::glow_desc_.hit_points = 1;
  Control43::glow_desc_.layer = 12;
  return 1;
}

// 0x4140d0
int Control42::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(50));
  actor->debris = 17;
  OnDamage(actor, kEventAim);
  return 1;
}

// 0x414130
int Control42::OnMove(Actor* actor) {
  Control42Work* work = reinterpret_cast<Control42Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  switch (work->step) {
    case 0:
      work->laser = g_actor_system.Create(43, actor, 0.0f, -16.0f, 6, 0, work);
      if (work->laser == NULL) {
        OnDamage(actor, kEventAim);
        return 1;
      }
      ++work->step;
      work->timer = work->duration;
      // Fall through.
    case 1:
      if (--work->timer <= 0) {
        if (work->laser->control != NULL) {
          work->laser->control->OnEvent(work->laser, 2, 0);
        }
        ++work->step;
        work->laser = NULL;
        work->timer = 10;
      }
      break;
    case 2:
      if (--work->timer <= 0) {
        Schedule(actor, work->delay, kEventAim);
      }
      break;
  }
  return 1;
}

// Actor::work of control 44.
struct Control44Work : PartWork {
  short interval;  // 0x54
  short delay;     // 0x56
};

// 0x414650
void Control44::Setup(Actor* actor, int rank) {
  Control44Work* work = reinterpret_cast<Control44Work*>(actor->work);
  short interval = rank / 35 * 8 + 8;
  work->interval = interval;
  work->delay = 120 - rank * 60 / 100;
  work->volley_time = interval;
}

// 0x414700
int Control44::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  reinterpret_cast<PartWork*>(actor->work)->flags &= ~PartWork::kAims;
  actor->SetSprite(g_pats.sys[0].Get(51));
  actor->debris = 17;
  OnDamage(actor, kEventIdle);
  return 1;
}

// 0x414760
int Control44::OnMove(Actor* actor) {
  Control44Work* work = reinterpret_cast<Control44Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  switch (work->step) {
    case 0:
      work->timer = work->interval;
      ++work->step;
      // Fall through.
    case 1:
      if (!(work->timer & 7) && g_actor_system.count() < 700) {
        g_actor_system.Create(45, NULL, actor->screen.m[2][0],
                              actor->screen.m[2][1], 11, actor->ScreenAngle(),
                              work);
        g_application.sound->channel(22)->Assign(&g_sounds[kSndFireMs], 0,
                                                 1.0f);
      }
      if (--work->timer <= 0) {
        Schedule(actor, work->delay, kEventIdle);
      }
      break;
  }
  return 1;
}

// Actor::work of control 45.
struct Control45Work {
  EnmSlot* slot;       // 0x30, of the launcher
  unsigned int flags;  // 0x34
  PartWork* launcher;  // 0x38
  short homing;        // 0x3c, frames it still turns to the player
};

// 0x414830
int Control45::OnCreate(Actor* actor, void* param) {
  Prepare(actor, param);
  Control45Work* work = reinterpret_cast<Control45Work*>(actor->work);
  work->launcher = static_cast<PartWork*>(param);
  actor->team = 0x61;
  actor->debris = 17;
  actor->SetSprite(g_pats.sys[0].Get(72));
  actor->speed = work->launcher->rank * 0.014285714f + 4.0f;
  actor->angle = actor->start_angle;
  work->slot = work->launcher->slot;
  work->homing = work->launcher->rank * 180 / 100 + 40;
  return 1;
}

// 0x414ac0
int Control45::OnMove(Actor* actor) {
  Control45Work* work = reinterpret_cast<Control45Work*>(actor->work);
  if (actor->age > 30 && work->homing > 0) {
    actor->TurnTo(g_player, 8);
    actor->start_angle = actor->angle;
    --work->homing;
  }
  if (IsOffScreen(actor)) {
    work->homing = 0;
  }
  if (actor->age & 1) {
    g_actor_system.Create(21, NULL, actor->screen.m[2][0],
                          actor->screen.m[2][1], 12, 0, NULL);
  }
  return 1;
}

// Actor::work of control 21, the smoke of a missile.
struct SmokeWork {
  short alpha;  // 0x30
                // 0x32: padding
  float scale;  // 0x34
};

// The colors of the smoke by age (0: keep the color).
D3DCOLOR g_smoke_colors[21] = {
    0xffffffff, 0, 0xfffefbb2, 0, 0xfffef97f, 0, 0, 0, 0xfffec15d, 0, 0, 0,
    0xffff8337, 0, 0,          0, 0xffc03d19, 0, 0, 0, 0,
};

// 0x4148d0
int Control21::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetSprite(g_pats.add[0].Get(24));
  actor->flags |= Actor::kAdditive;
  actor->color = 0xffffffff;
  SmokeWork* work = reinterpret_cast<SmokeWork*>(actor->work);
  work->scale = 0.5f;
  work->alpha = 0x80;
  actor->SetScale(0.5f);
  return 1;
}

// 0x414940
int Control21::OnMove(Actor* actor) {
  SmokeWork* work = reinterpret_cast<SmokeWork*>(actor->work);
  if (actor->age < 21) {
    if (g_smoke_colors[actor->age] != 0) {
      actor->color = g_smoke_colors[actor->age];
    }
    work->scale += 0.03f;
  } else if (actor->age == 21) {
    actor->SetSprite(g_pats.sys[0].Get(73));
    actor->flags &= ~Actor::kAdditive;
    actor->color = (work->alpha << 24) | 0xffffff;
    work->scale += 0.2f;
    actor->angle = rand() & 0x3ff;
  } else {
    work->alpha -= 4;
    if (work->alpha <= 0) {
      return 0;
    }
    actor->color = (work->alpha << 24) | 0xffffff;
  }
  actor->SetScale(work->scale);
  return 1;
}

// Actor::work of control 46.
struct Control46Work : PartWork {
  short count;  // 0x54, sparks at a time
  short base;   // 0x56, of the speed (tenths)
  short range;  // 0x58, of the speed (tenths)
  short delay;  // 0x5a
};

// 0x414b40
void Control46::Setup(Actor* actor, int rank) {
  Control46Work* work = reinterpret_cast<Control46Work*>(actor->work);
  work->count = rank * 25 / 100 + 5;
  work->range = (rank >> 2) + 30;
  work->base = 25;
  work->delay = 120 - rank * 60 / 100;
}

// 0x414ba0
int Control46::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.add[0].Get(25);
  desc_.sprite = sprite;
  desc_.flags = Actor::kAdditive;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 4.0f;
  desc_.layer = 12;
  desc_.debris = 0;
  return 1;
}

// 0x414c10
int Control46::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(52));
  actor->debris = 17;
  OnDamage(actor, kEventAim);
  return 1;
}

// 0x414c70
int Control46::OnMove(Actor* actor) {
  Control46Work* work = reinterpret_cast<Control46Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  if (g_actor_system.count() < 700) {
    for (int i = 0; i < work->count; ++i) {
      desc_.work = reinterpret_cast<int>(work->slot);
      float x = actor->screen.m[2][0];
      desc_.x = x + (rand() & 7) - 4.0f;
      float y = actor->screen.m[2][1];
      desc_.y = y + (rand() & 7) - 4.0f;
      desc_.start_angle = actor->ScreenAngle() + (rand() & 0x1f) - 16;
      desc_.angle = rand() & 0x3ff;
      desc_.speed = (rand() % work->range + work->base) * 0.1f;
      g_actor_system.Spawn(&desc_);
    }
    g_application.sound->channel(22)->Assign(&g_sounds[kSndFireMs], 0, 1.0f);
  }
  Schedule(actor, work->delay, kEventAim);
  return 1;
}

// Actor::work of control 49.
struct Control49Work : PartWork {
  int start;        // 0x54, the age when the sweep started
  short sweep;      // 0x58, frames of the sweep
  short delay;      // 0x5a
  short amplitude;  // 0x5c, of the sweep
};

// 0x414dc0
void Control49::Setup(Actor* actor, int rank) {
  Control49Work* work = reinterpret_cast<Control49Work*>(actor->work);
  short sweep = rank * 240 / 100 + 120;
  work->sweep = sweep;
  work->delay = 180 - rank * 120 / 100;
  work->volley_time = sweep;
}

// 0x414e20
int Control49::Initialize() {
  memset(&desc_, 0, sizeof(desc_));
  desc_.color = 0xffffffff;
  HLSObject* sprite = g_pats.sys[0].Get(66);
  desc_.sprite = sprite;
  desc_.flags = Actor::kFaceStart;
  desc_.team = 9;
  desc_.hit_points = 1;
  desc_.speed = 6.0f;
  desc_.layer = 13;
  return 1;
}

// 0x414e80
int Control49::OnCreate(Actor* actor, void* param) {
  InitCore(actor, param);
  actor->SetSprite(g_pats.sys[0].Get(54));
  actor->debris = 17;
  Control49Work* work = reinterpret_cast<Control49Work*>(actor->work);
  work->flags &= ~PartWork::kAims;
  work->amplitude = (work->high - work->low) / 2;
  work->angle = work->low + work->amplitude;
  work->turret->start_angle = work->angle;
  work->turret->angle = work->turret->start_angle;
  OnDamage(actor, kEventIdle);
  return 1;
}

// 0x414f20
int Control49::OnMove(Actor* actor) {
  Control49Work* work = reinterpret_cast<Control49Work*>(actor->work);
  if (work->state != kEventFire) {
    return MovePart(actor);
  }
  switch (work->step) {
    case 0:
      work->start = actor->age;
      work->timer = work->sweep;
      ++work->step;
      // Fall through.
    case 1:
      work->turret->angle =
          static_cast<short>(
              work->amplitude *
              g_sin_table[((actor->age - work->start) * 2) & 0x3ff]) +
          work->angle;
      if (!(work->timer & 7) && g_actor_system.count() < 700) {
        desc_.x = actor->screen.m[2][0];
        desc_.y = actor->screen.m[2][1];
        desc_.start_angle = actor->ScreenAngle();
        desc_.work = reinterpret_cast<int>(work->slot);
        g_actor_system.Spawn(&desc_);
        g_application.sound->channel(17)->Assign(&g_sounds[kSndFireBl], 0,
                                                 1.0f);
      }
      if (--work->timer <= 0) {
        OnDamage(actor, kEventReturn);
        work->timer = work->delay;
      }
      break;
  }
  return 1;
}

// Control 43 -----------------------------------------------------------------

// The animations of the beam by Control42Work::mode.
short g_beam_scripts[3][4] = {
    {17, 40, 18, -1},
    {17, 40, 19, -1},
    {17, 40, 20, -1},
};
AnmData g_beam_anims[3] = {
    {reinterpret_cast<AnmSet*>(&g_pats.add[0]), g_beam_scripts[0]},
    {reinterpret_cast<AnmSet*>(&g_pats.add[0]), g_beam_scripts[1]},
    {reinterpret_cast<AnmSet*>(&g_pats.add[0]), g_beam_scripts[2]},
};
// The beam stops after 8 segments outside this rectangle.
FloatRect g_beam_bounds = {-120.0f, -120.0f, 760.0f, 600.0f};
// The width of the beam by Control42Work::mode.
float g_beam_scales[3] = {1.0f, 1.5f, 2.0f};

// 0x414210
int Control43::OnCreate(Actor* actor, void* param) {
  Control43Work* work = reinterpret_cast<Control43Work*>(actor->work);
  work->owner = static_cast<Control42Work*>(param);
  work->sound = 0;
  actor->hit_points = 1;
  actor->team = 0;
  beam_desc_.work = reinterpret_cast<int>(work->owner->slot);
  beam_desc_.y = -24.0f;
  beam_desc_.parent = actor;
  beam_desc_.sprite = g_anm_sys.Play(&g_beam_anims[work->owner->mode]);
  work->head = g_actor_system.Spawn(&beam_desc_);
  if (work->head == NULL) {
    return 0;
  }
  int angle = static_cast<short>(actor->ScreenAngle());
  float dx = g_sin_table[angle & 0x3ff] * 42.0f;
  float dy = g_sin_table[(angle + 0x100) & 0x3ff] * -42.0f;
  float x = actor->parent->screen.m[2][0];
  float y = actor->parent->screen.m[2][1];
  beam_desc_.y = -42.0f;
  beam_desc_.parent = work->head;
  for (int i = 0; i < 20; ++i) {
    beam_desc_.sprite = g_anm_sys.Play(&g_beam_anims[work->owner->mode]);
    if (g_actor_system.Spawn(&beam_desc_) == NULL) {
      return 0;
    }
    beam_desc_.y -= 42.0f;
    x += dx;
    y += dy;
    if (i > 8 && (x < g_beam_bounds.left || x > g_beam_bounds.right ||
                  y < g_beam_bounds.top || y > g_beam_bounds.bottom)) {
      break;
    }
  }
  glow_desc_.parent = actor;
  work->glow = g_actor_system.Spawn(&glow_desc_);
  if (work->glow == NULL) {
    return 0;
  }
  work->scale = g_beam_scales[work->owner->mode];
  work->glow->SetScale(work->scale);
  work->timer = 0;
  g_application.sound->channel(19)->Assign(&g_sounds[kSndFireLs1], 0, 1.0f);
  ++sound_users_[0];
  work->sound = 0;
  return 1;
}

// 0x414480
int Control43::OnMove(Actor* actor) {
  Control43Work* work = reinterpret_cast<Control43Work*>(actor->work);
  // The beam flickers.
  int offset = -6 - (rand() & 0x1f);
  Actor* head = work->head;
  head->transform->m[2][1] = static_cast<float>(offset);
  head->flags |= Actor::kMoved;
  work->glow->SetScale(((rand() & 0x1f) + 64) * work->scale / 64.0f);
  if (actor->age == 40) {
    if (--sound_users_[0] == 0) {
      g_application.sound->channel(19)->Stop();
    }
    g_application.sound->channel(20)->Assign(&g_sounds[kSndFireLs2], 0, 1.0f);
    ++sound_users_[1];
    work->sound = 1;
  }
  if (work->timer != 0) {
    if (--work->timer <= 0) {
      return 0;
    }
  }
  return 1;
}

// 0x414550
void Control43::OnEvent(Actor* actor, int x, int y) {
  Control43Work* work = reinterpret_cast<Control43Work*>(actor->work);
  if (x != 2 || work->sound == 2) {
    return;
  }
  // The beam turns into its fading sprite.
  Actor* segment = work->head;
  segment->SetSprite(g_pats.add[0].Get(17));
  for (segment = segment->first_child; segment != NULL;
       segment = segment->prev_sibling) {
    segment->SetSprite(g_pats.add[0].Get(17));
  }
  work->scale = 1.0f;
  switch (work->sound) {
    case 0:
      if (--sound_users_[0] == 0) {
        g_application.sound->channel(19)->Stop();
      }
      break;
    case 1:
      if (--sound_users_[1] == 0) {
        g_application.sound->channel(20)->Stop();
      }
      break;
  }
  g_application.sound->channel(21)->Assign(&g_sounds[kSndFireLs3], 0, 1.0f);
  work->timer = 10;
  work->sound = 2;
}
