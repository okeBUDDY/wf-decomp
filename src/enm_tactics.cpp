// The battle tactics of CEnmParam (tactic_program_).
#include <stdlib.h>

#include "enm_param.h"

// 0x4161d0
int CEnmParam::TacticRoam(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      if (state_.tactic_count > 5) {
        tactic_program_.arg = 1;
      } else {
        tactic_program_.arg = rand() % 3 + 1;
      }
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
      break;
    case 1:
      if (move_program_.op == 0) {
        if (IsSideBeaten(boss, 0)) {
          return 1;
        }
        if (--tactic_program_.arg <= 0) {
          return 1;
        }
        if (rand() & 7) {
          move_program_.Start(EnmProgram::kMoveRandom, 0);
        } else {
          move_program_.Start(EnmProgram::kMoveRam, 0);
        }
      }
      break;
  }
  return 0;
}

// 0x4162f0
int CEnmParam::TacticDashFlip(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 70);
      fire_programs_[1].Start(EnmProgram::kFireAny, 70);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      if (IsSideBeaten(boss, 50)) {
        turn_program_.Start(EnmProgram::kTurnFlip, 0);
      }
      ++tactic_program_.step;
      break;
    case 1:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      if (move_program_.op == 0) {
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      break;
  }
  return 0;
}

// 0x416560
int CEnmParam::TacticCharge(Actor* boss) {
  RunOpening(boss, 10);
  switch (tactic_program_.step) {
    case 10:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      turn_program_.Start(EnmProgram::kTurnTo, 0);
      ++tactic_program_.step;
      break;
    case 11:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x416630
int CEnmParam::TacticRearSparks(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 50);
      fire_programs_[1].op = 0;
      tactic_program_.SetArgAndNext(rand() % 3 + 3);
      // Fall through.
    case 1:
      if (core_power_[1][4] == 0) {
        return 1;
      }
      turn_program_.Start(EnmProgram::kTurnFace, 0x200);
      ++tactic_program_.step;
      break;
    case 2:
      if (turn_program_.op == 0 || (state_.flags & EnmState::kTurned)) {
        fire_programs_[1].Start(EnmProgram::kFireBoosted, 4);
        tactic_program_.Wait(60);
      }
      break;
    case 3:
      if (tactic_program_.time <= 0 && turn_program_.op == 0) {
        if (--tactic_program_.arg <= 0) {
          turn_program_.Start(EnmProgram::kTurnTo, 0);
          ++tactic_program_.step;
        } else {
          tactic_program_.step = 1;
        }
      }
      break;
    case 4:
      if (turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4167a0
int CEnmParam::TacticStream(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      if (core_power_[0][6] <= 200 && core_power_[1][6] <= 200) {
        return 1;
      }
      fire_programs_[0].Start(EnmProgram::kFireBoostedWait, 6);
      fire_programs_[1].Start(EnmProgram::kFireBoostedWait, 6);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
      // Fall through.
    case 1:
      if (move_program_.op == 0) {
        if (fire_programs_[0].op == 0 && fire_programs_[1].op == 0) {
          return 1;
        }
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      if (core_power_[0][6] <= 0 && core_power_[1][6] <= 0) {
        fire_programs_[0].Start(EnmProgram::kFireAny, 100);
        fire_programs_[1].Start(EnmProgram::kFireAny, 100);
        ++tactic_program_.step;
      }
      break;
    case 2:
      if (move_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4168e0
int CEnmParam::TacticRapidFire(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 200);
      fire_programs_[1].Start(EnmProgram::kFireAny, 200);
      tactic_program_.time = (rand() % 10 + 5) * 60;
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
      // Fall through.
    case 1:
      if (move_program_.op == 0) {
        if (core_count_ < 5 || tactic_program_.time <= 0) {
          return 1;
        }
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      break;
  }
  return 0;
}

// 0x4169c0
int CEnmParam::TacticSpinDash(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 70);
      fire_programs_[1].Start(EnmProgram::kFireAny, 70);
      turn_program_.Start(EnmProgram::kTurnSpin, (rand() % 20 + 20) * 60);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
      break;
    case 1:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      if (move_program_.op == 0) {
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      break;
  }
  return 0;
}

// 0x416ac0
int CEnmParam::TacticChargeRearSparks(Actor* boss) {
  RunOpening(boss, 10);
  switch (tactic_program_.step) {
    case 10:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].op = 0;
      tactic_program_.arg = rand() % 3 + 3;
      // Fall through.
    case 11:
      if (tactic_program_.arg > 0 && core_power_[1][4] > 0) {
        turn_program_.Start(EnmProgram::kTurnFace, 0x200);
        ++tactic_program_.step;
      } else {
        tactic_program_.step = 20;
      }
      break;
    case 12:
      if (turn_program_.op == 0 || (state_.flags & EnmState::kTurned)) {
        fire_programs_[1].Start(EnmProgram::kFireBoosted, 4);
        tactic_program_.Wait(60);
      }
      break;
    case 13:
      if (tactic_program_.time <= 0 && turn_program_.op == 0) {
        --tactic_program_.arg;
        tactic_program_.step = 11;
      }
      break;
    case 20:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      turn_program_.Start(EnmProgram::kTurnTo, 0);
      ++tactic_program_.step;
      // Fall through.
    case 21:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x416cb0
int CEnmParam::TacticChargeFlip(Actor* boss) {
  RunOpening(boss, 10);
  switch (tactic_program_.step) {
    case 10:
      fire_programs_[0].Start(EnmProgram::kFireAny, 200);
      fire_programs_[1].Start(EnmProgram::kFireAny, 200);
      turn_program_.Start(EnmProgram::kTurnFlip, 0);
      ++tactic_program_.step;
      // Fall through.
    case 11:
      if (turn_program_.op == 0) {
        tactic_program_.step = 20;
      }
      break;
    case 20:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      turn_program_.Start(EnmProgram::kTurnTo, 0);
      ++tactic_program_.step;
      // Fall through.
    case 21:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x416e10
int CEnmParam::TacticRepeatCharge(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      tactic_program_.arg = rand() % 3 + 3;
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      turn_program_.Start(EnmProgram::kTurnFace, 0);
      ++tactic_program_.step;
      // Fall through.
    case 1:
      if (turn_program_.op == 0 || (state_.flags & EnmState::kTurned)) {
        fire_programs_[0].op = 0;
        fire_programs_[1].op = 0;
        move_program_.Start(EnmProgram::kMoveCharge, 0);
        ++tactic_program_.step;
      }
      break;
    case 2:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        tactic_program_.Wait(60);
      }
      break;
    case 3:
      if (tactic_program_.time <= 0) {
        --tactic_program_.arg;
        fire_programs_[0].Start(EnmProgram::kFireAny, 100);
        fire_programs_[1].Start(EnmProgram::kFireAny, 100);
        if (tactic_program_.arg <= 0) {
          move_program_.Start(EnmProgram::kMoveRandom, 0);
          turn_program_.Start(EnmProgram::kTurnTo, 0);
          ++tactic_program_.step;
        } else {
          turn_program_.Start(EnmProgram::kTurnFace, 0);
          tactic_program_.step = 1;
        }
      }
      break;
    case 4:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x417010
int CEnmParam::TacticSpin(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 200);
      fire_programs_[1].Start(EnmProgram::kFireAny, 200);
      turn_program_.Start(EnmProgram::kTurnSpin, (rand() % 20 + 20) * 60);
      ++tactic_program_.step;
      break;
    case 1:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4170c0
int CEnmParam::TacticLastStand(Actor* boss) {
  switch (tactic_program_.step) {
    case 0:
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      // Fall through.
    case 1:
      if (turn_program_.op == 0) {
        turn_program_.Start(EnmProgram::kTurnTo, 0);
        ++tactic_program_.step;
      }
      break;
  }
  if (move_program_.op == 0) {
    move_program_.Start(EnmProgram::kMoveRandom, 300);
  }
  return 0;
}

// 0x417430
int CEnmParam::TacticGlowArmor(Actor* boss) {
  switch (tactic_program_.step) {
    case 0: {
      int time = (rand() % 5 + 5) * 60;
      if (!GlowArmor(boss, time)) {
        return 1;
      }
      fire_programs_[0].Start(EnmProgram::kFireAny, 70);
      fire_programs_[1].Start(EnmProgram::kFireAny, 70);
      tactic_program_.time = time;
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
    }
      // Fall through.
    case 1:
      if (tactic_program_.time <= 0) {
        if (move_program_.op == 0) {
          return 1;
        }
      } else if (move_program_.op == 0) {
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      break;
  }
  return 0;
}

// 0x417520
int CEnmParam::TacticTransform(Actor* boss) {
  switch (tactic_program_.step) {
    case 2:
      if (tactic_program_.time <= 0) {
        return 1;
      }
      break;
    case 1:
      if (trans_count_ <= 0) {
        tactic_program_.Wait(90);
      }
      break;
    case 0:
      if (!StartTransform(boss)) {
        return 1;
      }
      fire_programs_[0].Start(EnmProgram::kFireAny, 70);
      fire_programs_[1].Start(EnmProgram::kFireAny, 70);
      ++tactic_program_.step;
      break;
  }
  return 0;
}

// 0x4175d0
int CEnmParam::TacticSpinGlowSparks(Actor* boss) {
  switch (tactic_program_.step) {
    case 0: {
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].op = 0;
      int time = (rand() % 20 + 20) * 60;
      GlowArmor(boss, time);
      turn_program_.Start(EnmProgram::kTurnSpin, time);
      tactic_program_.Wait(60);
      break;
    }
    case 1:
      if (turn_program_.op == 0) {
        return 1;
      }
      if (tactic_program_.time <= 0) {
        if (core_power_[1][4] > 0) {
          fire_programs_[1].Start(EnmProgram::kFireBoosted, 4);
          tactic_program_.time = 60;
        } else {
          fire_programs_[1].Start(EnmProgram::kFireAny, 100);
          tactic_program_.time = 60;
        }
      }
      break;
  }
  return 0;
}

// 0x4176f0
int CEnmParam::TacticGlowRapidFire(Actor* boss) {
  switch (tactic_program_.step) {
    case 0: {
      fire_programs_[0].Start(EnmProgram::kFireAny, 200);
      fire_programs_[1].Start(EnmProgram::kFireAny, 200);
      int time = (rand() % 10 + 5) * 60;
      GlowArmor(boss, time);
      tactic_program_.time = time;
      move_program_.Start(EnmProgram::kMoveRandom, 0);
      ++tactic_program_.step;
    }
      // Fall through.
    case 1:
      if (tactic_program_.time <= 0) {
        if (move_program_.op == 0) {
          return 1;
        }
      } else if (move_program_.op == 0) {
        move_program_.Start(EnmProgram::kMoveRandom, 0);
      }
      break;
  }
  return 0;
}

// 0x4177e0
int CEnmParam::TacticSpinRam(Actor* boss) {
  switch (tactic_program_.step) {
    case 0: {
      fire_programs_[0].Start(EnmProgram::kFireAny, 100);
      fire_programs_[1].Start(EnmProgram::kFireAny, 100);
      int time = (rand() % 20 + 20) * 60;
      GlowArmor(boss, time);
      turn_program_.Start(EnmProgram::kTurnSpin, time);
      move_program_.Start(EnmProgram::kMoveRam, 0);
      ++tactic_program_.step;
      break;
    }
    case 1:
      if (move_program_.op == 0 && turn_program_.op == 0) {
        return 1;
      }
      if (move_program_.op == 0) {
        move_program_.Start(EnmProgram::kMoveRam, 0);
      }
      break;
  }
  return 0;
}
