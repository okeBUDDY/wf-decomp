// The helpers of the battle tactics. They are compiled apart from the
// tactics (enm_tactics.cpp): within one file VC knows the registers a
// function leaves alone, and the tactics would then keep `this` in ecx
// across the calls.
#include "enm_param.h"

// 0x416170
int CEnmParam::IsSideBeaten(int unused, int percent) {
  if (state_.step_mask & 2) {
    int left = parts_alive_[0] * 100 / unknown_f4_[0];
    int right = parts_alive_[1] * 100 / unknown_f4_[1];
    if (unknown_14c_ == 2) {
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
// TODO: nonmatching (VC folds the step of case 2 into a store of 3; the
// original increments it)
void CEnmParam::RunOpening(int unused, int next_step) {
  if (list_218_.step >= next_step) {
    return;
  }
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      list_218_.arg = 1;
      list_b0_.Start(0x20004, 0);
      ++list_218_.step;
      // Fall through.
    case 1:
      if (list_b0_.op != 0 && !(state_.flags & 2)) {
        break;
      }
      if (unknown_14c_ == 2 && list_218_.arg > 0) {
        if (list_b0_.op == 0) {
          --list_218_.arg;
          list_b0_.Start(0x20004, 0);
        }
        break;
      }
      lists_c4_[0].op = 0;
      lists_c4_[1].op = 0;
      list_9c_.Start(0x10003, 0);
      ++list_218_.step;
      break;
    case 2:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        list_218_.time = 60;
        list_218_.step++;
      }
      break;
    case 3:
      if (list_218_.time <= 0) {
        list_218_.step = next_step;
      }
      break;
  }
}
