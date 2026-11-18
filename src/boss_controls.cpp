// The boss: its body (control 32), its linked parts (33, 34) and ...
#include "actor_control.h"

#include <math.h>
#include <stdlib.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "game.h"
#include "game_data.h"
#include "hl_3d.h"
#include "snd_sys.h"
#include "sounds.h"

// 0x411500
int Control32::Initialize() {
  bounds_.left = 64.0f;
  bounds_.top = 64.0f;
  bounds_.right = 576.0f;
  bounds_.bottom = 416.0f;
  return 1;
}

// 0x411530
int Control32::GetScore(Actor* actor) {
  return (g_global.stage * 10 + g_enm_param.state_.unknown_24_ - 10) * 1000;
}

// 0x412b90
int Control34::OnCreate(Actor* actor, void* param) {
  InitLinked(actor, param);
  PartWork* work = reinterpret_cast<PartWork*>(actor->work);
  unsigned int base = CEnmParam::ChildBase(work->slot->code);
  int count =
      work->slot->part->extra_child_count + work->slot->part->child_count;
  SpawnChildren(actor, base, 0, count);
  if (work->slot->flags & 0x3000) {
    SpawnChildren(actor, base, count, work->slot->part->option_count);
  }
  if (work->slot->flags & 0x4000) {
    SpawnChildren(actor, base, work->slot->part->option_count + count,
                  work->slot->part->option4_count);
  }
  ScaleHitPoints(actor);
  SpawnItems(actor);
  return 1;
}

// 0x412c30
int Control33::OnCreate(Actor* actor, void* param) {
  InitLinked(actor, param);
  SpawnChildren(actor,
                CEnmParam::ChildBase(
                    reinterpret_cast<PartWork*>(actor->work)->slot->code),
                1, 2);
  ScaleHitPoints(actor);
  return 1;
}

// Actor::work of control 32, the body of the boss.
struct Control32Work {
  enum Flags {
    kDashing = 0x1,  // in PartWork::flags
  };

  char part_[0x20];    // 0x30, as PartWork (only its first 0x20 bytes)
  Actor* arms[4];      // 0x50, the parts on the left (0, 1) and right (2, 3)
  float deceleration;  // 0x60, of the dash
  short brake_time;    // 0x64, frames into the dash it starts to brake, 0:
                       // braking
};

// The pieces of the body.
extern PartPiece g_body_pieces[4];  // 0x45d418

// Mirrors `actor` by `flag` (Actor::kMirrorX or kMirrorY).
static inline void Mirror(Actor* actor, unsigned int flag) {
  actor->flags |= flag;
  actor->matrix_angle += 0x1000;
}

// 0x4116f0
int Control32::OnCreate(Actor* actor, void* param) {
  Control32Work* work = reinterpret_cast<Control32Work*>(actor->work);
  boss_ = actor;
  InitLinked(actor, param);
  actor->hit_points = g_enm_param.state_.unknown_24_;
  actor->team = 0x21;
  actor->debris = 20;
  actor->SetSprite(g_enemy_pats.Get(1));
  actor->bounds = &bounds_;
  reinterpret_cast<PartWork*>(actor->work)->max_hit_points = actor->hit_points;
  EnmSlot* slot = g_enm_param.FindSlot(0x40000000);
  const EnmPlanPart* part = slot->part;
  work->arms[0] =
      g_actor_system.Create(33, actor, part->x, part->y, 4, 0, slot);
  work->arms[1] =
      g_actor_system.Create(33, actor, -1 - part->x, part->y, 4, 0, slot);
  Mirror(work->arms[1], Actor::kMirrorX);
  slot = g_enm_param.FindSlot(0x80000000);
  part = slot->part;
  work->arms[2] =
      g_actor_system.Create(33, actor, part->x1, part->y1, 4, 0, slot);
  Mirror(work->arms[2], Actor::kMirrorY);
  work->arms[3] =
      g_actor_system.Create(33, actor, -1 - part->x1, part->y1, 4, 0, slot);
  Mirror(work->arms[3], Actor::kMirrorX);
  Mirror(work->arms[3], Actor::kMirrorY);
  for (int i = 0; i < 4; ++i) {
    reinterpret_cast<PartWork*>(actor->work)->level +=
        reinterpret_cast<PartWork*>(work->arms[i]->work)->level;
  }
  ScaleHitPoints(actor);
  SpawnPieces(actor, g_body_pieces, 4);
  bounds_.top = actor->transform->m[2][1];
  g_enm_param.list_9c_.op = 0x10001;
  g_enm_param.list_9c_.arg = 0;
  g_enm_param.list_9c_.step = 0;
  g_enm_param.list_9c_.time = 0;
  return 1;
}

// Passes the event (`x`, `y`) on to `actor`, if any.
static inline void SendEvent(Actor* actor, int x, int y) {
  if (actor != NULL && actor->control != NULL) {
    actor->control->OnEvent(actor, x, y);
  }
}

// 0x411d10
void Control32::OnEvent(Actor* actor, int x, int y) {
  Control32Work* work = reinterpret_cast<Control32Work*>(actor->work);
  if (!(y & 0x400)) {
    SendEvent(work->arms[0], x, y);
    SendEvent(work->arms[1], x, y);
  }
  if (!(y & 0x200)) {
    SendEvent(work->arms[2], x, y);
    SendEvent(work->arms[3], x, y);
  }
}

// 0x411e20
// TODO: nonmatching (the original keeps the squared distance on the FPU stack
// for the test and spills the acceleration)
int Control32::Dash(Actor* actor, EnmList* program, float x, float y,
                    float power, int boost) {
  Control32Work* work = reinterpret_cast<Control32Work*>(actor->work);
  Transform* transform = actor->transform;
  float from_x = transform->m[2][0];
  float from_y = transform->m[2][1];
  if (x < bounds_.left) {
    y = (bounds_.left - from_x) * (y - from_y) / (x - from_x) + from_y;
    x = bounds_.left;
  } else if (x > bounds_.right) {
    y = (bounds_.right - from_x) * (y - from_y) / (x - from_x) + from_y;
    x = bounds_.right;
  }
  if (y < bounds_.top) {
    x = (bounds_.top - from_y) * (x - from_x) / (y - from_y) + from_x;
    y = bounds_.top;
  } else if (y > bounds_.bottom) {
    x = (bounds_.bottom - from_y) * (x - from_x) / (y - from_y) + from_x;
    y = bounds_.bottom;
  }
  float dx = x - transform->m[2][0];
  float dy = y - transform->m[2][1];
  float distance2 = dy * dy + dx * dx;
  float acceleration = power * 0.005f;
  work->deceleration = acceleration;
  if (!(distance2 > 0.0f)) {
    return 0;
  }
  float speed = sqrt(sqrt(distance2) * acceleration * 2.0f /
                     ((acceleration + acceleration) * acceleration));
  work->brake_time = static_cast<short>(speed / acceleration * acceleration);
  if (work->brake_time <= 0) {
    work->brake_time = 1;
  }
  actor->acceleration = acceleration;
  actor->speed = 0.0f;
  actor->start_angle = actor->AngleTo(x, y);
  program->time = 0;
  reinterpret_cast<PartWork*>(actor->work)->flags |= Control32Work::kDashing;
  if (boost) {
    g_enm_param.unknown_164_ = 0;
    g_enm_param.unknown_158_ = actor->start_angle;
    g_enm_param.flare_time_ = work->brake_time;
    g_enm_param.flare_size_ = power;
    for (int i = 0; i < 4; ++i) {
      SendEvent(work->arms[i], 9, 0x110a);
    }
    if (power > 1.0f && work->brake_time > 90) {
      g_application.sound->channel(26)->Assign(&g_snd_boost2, 0, 1.0f);
      return 1;
    }
    g_application.sound->channel(25)->Assign(&g_snd_boost1, 0, 1.0f);
  }
  return 1;
}

// The power of the dash of `program`: its argument in percent.
static __forceinline float DashPower(const EnmList* program) {
  return program->arg > 0 ? program->arg * 0.01f : 1.0f;
}

// 0x4120d0
// TODO: nonmatching (register choice; the original merges the Dash() calls of
// the cases into one call site)
void Control32::RunProgram9c(Actor* actor, EnmList* program) {
  Control32Work* work = reinterpret_cast<Control32Work*>(actor->work);
  if (reinterpret_cast<PartWork*>(actor->work)->flags &
      Control32Work::kDashing) {
    if (work->brake_time != 0 && program->time >= work->brake_time) {
      actor->acceleration = -work->deceleration;
      work->brake_time = 0;
    }
    if (!(actor->speed > 0.0f)) {
      reinterpret_cast<PartWork*>(actor->work)->flags &=
          ~Control32Work::kDashing;
      program->op = 0;
      ++program->time;
      return;
    }
  } else {
    switch (program->op) {
      case 0x10002:
        if (!Dash(actor, program, rand() % 320 - 160 + 320.0f,
                  rand() % 100 - 50 + 160.0f, DashPower(program), 0)) {
          program->op = 0;
          ++program->time;
          return;
        }
        break;
      case 0x10005: {
        float power = DashPower(program);
        float x;
        float y;
        if (g_player != NULL) {
          x = g_player->transform->m[2][0];
          y = g_player->transform->m[2][1];
        } else {
          x = 320.0f;
          y = 400.0f;
        }
        if (!Dash(actor, program, x, y, power, 1)) {
          program->op = 0;
          ++program->time;
          return;
        }
        break;
      }
      case 0x10004:
        if (!Dash(actor, program, 320.0f, 160.0f, DashPower(program), 0)) {
          program->op = 0;
          ++program->time;
          return;
        }
        break;
      case 0x10003: {
        float power = DashPower(program);
        Transform* transform = actor->transform;
        float x = g_sin_table[(actor->angle - 0x200) & 0x3ff] * 4096.0f +
                  transform->m[2][0];
        float y = transform->m[2][1] -
                  g_sin_table[(actor->angle - 0x100) & 0x3ff] * 4096.0f;
        if (!Dash(actor, program, x, y, power * 6.0f, 1)) {
          program->op = 0;
          ++program->time;
          return;
        }
        break;
      }
      case 0x10006:
        actor->start_angle = program->arg;
        actor->speed = 1.6f;
        actor->acceleration = -0.02f;
        reinterpret_cast<PartWork*>(actor->work)->flags |=
            Control32Work::kDashing;
        work->brake_time = 0;
        break;
      case 0x10001:
        switch (program->step) {
          case 0:
            actor->start_angle = 0x200;
            actor->acceleration = 0.0f;
            actor->speed = 5.0f;
            ++program->step;
          case 1:
            if (!(actor->speed > 0.0f)) {
              g_enm_param.state_.flags |= 0x100;
              bounds_.top = 64.0f;
              program->op = 0;
              ++program->time;
              return;
            }
            if (actor->transform->m[2][1] > 0.0f &&
                actor->acceleration == 0.0f) {
              actor->acceleration = actor->speed * -0.016666668f;
            }
            break;
        }
        break;
    }
  }
  ++program->time;
}

// 0x412750
int Control32::OnMove(Actor* actor) {
  if (g_enm_param.list_9c_.op != 0) {
    RunProgram9c(actor, &g_enm_param.list_9c_);
  }
  if (g_enm_param.list_b0_.op != 0) {
    RunProgramB0(actor, &g_enm_param.list_b0_);
  }
  if (g_enm_param.lists_c4_[0].op != 0) {
    RunSideProgram(actor, &g_enm_param.lists_c4_[0], 0);
  }
  if (g_enm_param.lists_c4_[1].op != 0) {
    RunSideProgram(actor, &g_enm_param.lists_c4_[1], 1);
  }
  return 1;
}

// Actor::work of control 35, a turret of the boss.
struct Control35Work {
  EnmSlot* slot;  // 0x30
  short burst;    // 0x34, bullets left in the burst
  short weight;   // 0x36, of the bursts
  short angle;    // 0x38, of the next bullet
  short ring;     // 0x3a, fires rings
};

// 0x4127d0
void Control35::StartBurst(Actor* actor) {
  Control35Work* work = reinterpret_cast<Control35Work*>(actor->work);
  work->angle = rand() & 0x3ff;
  work->burst = (g_enm_param.core_total_ - g_enm_param.core_count_) *
                work->weight / g_enm_param.core_total_;
  work->burst = work->burst / g_enm_param.turret_count_;
  if (work->burst < 12) {
    work->burst = 12;
  }
}

// 0x412880
void Control35::FireRing(Actor* actor) {
  Control35Work* work = reinterpret_cast<Control35Work*>(actor->work);
  if (work->ring != 0 && g_enm_param.turret_count_ < 3 &&
      g_enm_param.core_count_ * 100 / g_enm_param.core_total_ < 50) {
    desc_.x = actor->screen.m[2][0];
    desc_.y = actor->screen.m[2][1];
    desc_.speed = 4.0f;
    desc_.work = reinterpret_cast<int>(work->slot);
    int angle = rand() % 64;
    for (int i = 16; i > 0; --i) {
      desc_.start_angle = angle;
      g_actor_system.Spawn(&desc_);
      g_application.sound->channel(22)->Assign(&g_snd_fire_ms, 0, 1.0f);
      angle += 0x40;
    }
  }
}

// 0x412950
int Control35::OnCreate(Actor* actor, void* param) {
  actor->hit_points = 1;
  actor->team = 0;
  actor->SetLayer(4);
  Control35Work* work = reinterpret_cast<Control35Work*>(actor->work);
  work->slot = static_cast<EnmSlot*>(param);
  work->weight = static_cast<short>(g_enm_param.unknown_98_ - work->slot->depth)
                 << 2;
  work->ring = work->slot->depth < g_enm_param.state_.max_depth >> 1;
  work->burst = 0;
  ++g_enm_param.turret_count_;
  return 1;
}

// 0x4129c0
int Control35::OnMove(Actor* actor) {
  Control35Work* work = reinterpret_cast<Control35Work*>(actor->work);
  if (work->burst != 0) {
    if (!(work->burst & 3)) {
      desc_.x = actor->screen.m[2][0];
      desc_.y = actor->screen.m[2][1];
      desc_.start_angle = work->angle;
      desc_.speed = 6.0f;
      desc_.work = reinterpret_cast<int>(work->slot);
      g_actor_system.Spawn(&desc_);
      g_application.sound->channel(17)->Assign(&g_snd_fire_bl, 0, 1.0f);
      work->angle += 20;
    }
    --work->burst;
    return 1;
  }
  switch (actor->age % 360) {
    case 60:
    case 120:
    case 180:
      StartBurst(actor);
      return 1;
    case 240:
    case 270:
    case 300:
      FireRing(actor);
      break;
  }
  return 1;
}
