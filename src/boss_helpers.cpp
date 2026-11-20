// Helpers of the boss body (control 32). They are compiled apart from
// boss_controls.cpp: within one file VC knows the registers a function
// leaves alone, and the callers would then keep `this` in ecx across the
// calls.
#include <math.h>
#include <stdlib.h>

#include "actor.h"
#include "actor_control.h"
#include "enm_param.h"
#include "game.h"
#include "hl_3d.h"

// 0x411580
int BossBodyControl::StartTurn(Actor* actor, EnmProgram* program, short angle) {
  BossBodyWork* work = reinterpret_cast<BossBodyWork*>(actor->work);
  work->turn_from = actor->angle;
  work->turn = angle - actor->angle;
  short size = abs(work->turn);
  if (size == 0) {
    return 0;
  }
  if (size > 90) {
    work->turn_peak = size;
    work->turn_end = size + 90;
  } else {
    // `angle` is done with and holds the peak.
    angle = static_cast<short>(sqrt(static_cast<double>(size * 90)));
    work->turn_peak = angle;
    work->turn_end = angle * 2;
  }
  reinterpret_cast<PartWork*>(actor->work)->flags |= BossBodyWork::kTurning;
  program->time = 0;
  return 1;
}

// 0x411600
int BossBodyControl::ListWeapons(int side, int* out) {
  int count = 0;
  for (int i = 0; i < 7; ++i) {
    if (g_enm_param.core_power_[side][i] != 0) {
      out[count++] = i;
    }
  }
  return count;
}

// Passes `event` for `targets` on to `actor`, if any.
static inline void SendEvent(Actor* actor, int event, int targets) {
  if (actor != NULL && actor->control != NULL) {
    actor->control->OnEvent(actor, event, targets);
  }
}

// 0x411960
void BossBodyControl::SignalSide(Actor* actor, int side, int event,
                                 int targets) {
  BossBodyWork* work = reinterpret_cast<BossBodyWork*>(actor->work);
  if (side != 0) {
    SendEvent(work->arms[2], event, targets);
    SendEvent(work->arms[3], event, targets);
  } else {
    SendEvent(work->arms[0], event, targets);
    SendEvent(work->arms[1], event, targets);
  }
}

// 0x4124f0
unsigned short BossBodyControl::AngleToPlayer(Actor* actor) {
  if (g_player != NULL) {
    return actor->AngleTo(g_player->screen.m[2][0], g_player->screen.m[2][1]);
  }
  return actor->AngleTo(320.0f, 400.0f);
}
