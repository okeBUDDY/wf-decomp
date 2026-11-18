// The boss: its body (control 32), its linked parts (33, 34) and ...
#include "actor_control.h"

#include "actor.h"
#include "cglobal.h"
#include "enm_param.h"

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
