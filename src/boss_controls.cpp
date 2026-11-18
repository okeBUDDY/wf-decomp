// The boss: its body (control 32), its linked parts (33, 34) and ...
#include "actor_control.h"

#include <stdlib.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
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
  char part_[0x20];  // 0x30, as PartWork
  Actor* arms[4];    // 0x50, the parts on the left (0, 1) and right (2, 3)
};

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

// 0x412750
int Control32::OnMove(Actor* actor) {
  if (g_enm_param.list_9c_.unknown_0c_ != 0) {
    RunProgram9c(actor, &g_enm_param.list_9c_);
  }
  if (g_enm_param.list_b0_.unknown_0c_ != 0) {
    RunProgramB0(actor, &g_enm_param.list_b0_);
  }
  if (g_enm_param.lists_c4_[0].unknown_0c_ != 0) {
    RunSideProgram(actor, &g_enm_param.lists_c4_[0], 0);
  }
  if (g_enm_param.lists_c4_[1].unknown_0c_ != 0) {
    RunSideProgram(actor, &g_enm_param.lists_c4_[1], 1);
  }
  return 1;
}

// Actor::work of control 35, a turret of the boss.
struct Control35Work {
  EnmSlot* slot;   // 0x30
  short burst;     // 0x34, bullets left in the burst
  short weight;    // 0x36, of the bursts
  short angle;     // 0x38, of the next bullet
  short ring;      // 0x3a, fires rings
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
  work->weight =
      static_cast<short>(g_enm_param.unknown_98_ - work->slot->depth) << 2;
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
