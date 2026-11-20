// The helpers of the battle tactics. They are compiled apart from the
// tactics (enm_tactics.cpp): within one file VC knows the registers a
// function leaves alone, and the tactics would then keep `this` in ecx
// across the calls.
#include "enm_param.h"

#include "actor.h"
#include "actor_control.h"

// 0x416170
int CEnmParam::IsSideBeaten(Actor* boss, int percent) {
  if (state_.tactic_mask & 2) {
    int left = parts_alive_[0] * 100 / start_parts_[0];
    int right = parts_alive_[1] * 100 / start_parts_[1];
    if (player_dir_ == 2) {
      if (left > right && right <= percent) {
        return 1;
      }
    } else {
      if (right > left && left <= percent) {
        return 1;
      }
    }
  }
  return 0;
}

// 0x4163f0
void CEnmParam::RunOpening(Actor* boss, int next_step) {
  if (tactic_program_.step >= next_step) {
    return;
  }
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 70);
      fire_programs_[1].Start(EnmProgram::kFireAny, 70);
      tactic_program_.arg = 1;
      turn_program_.Start(EnmProgram::kTurnFace, 0);
      ++tactic_program_.step;
      // Fall through.
    case 1:
      if (turn_program_.op != 0 && !(state_.flags & EnmState::kTurned)) {
        break;
      }
      if (player_dir_ == 2 && tactic_program_.arg > 0) {
        if (turn_program_.op == 0) {
          --tactic_program_.arg;
          turn_program_.Start(EnmProgram::kTurnFace, 0);
        }
        break;
      }
      fire_programs_[0].op = 0;
      fire_programs_[1].op = 0;
      move_program_.Start(EnmProgram::kMoveCharge, 0);
      ++tactic_program_.step;
      break;
    case 2:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        tactic_program_.Wait(60);
      }
      break;
    case 3:
      if (tactic_program_.time <= 0) {
        tactic_program_.step = next_step;
      }
      break;
  }
}

// 0x4173a0
int CEnmParam::GlowArmor(Actor* actor, int time) {
  if (armored_parts_[0] <= 0 && armored_parts_[1] <= 0) {
    return 0;
  }
  glow_time_ = time;
  if (actor->control != NULL) {
    actor->control->OnEvent(actor, PartControl::kEventGlow, 0x110a);
  }
  return 1;
}

// 0x4173f0
int CEnmParam::StartTransform(Actor* actor) {
  if (transform_parts_[0] <= 0 && transform_parts_[1] <= 0) {
    return 0;
  }
  if (actor->control != NULL) {
    actor->control->OnEvent(actor, PartControl::kEventTransform, 0xffff310a);
  }
  return 1;
}
