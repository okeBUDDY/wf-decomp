// The battle tactics of CEnmParam (list_218_).
#include <stdlib.h>

#include "enm_param.h"

// 0x4161d0
int CEnmParam::Tactic4161d0(int unused) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      if (state_.step_count > 5) {
        list_218_.arg = 1;
      } else {
        list_218_.arg = rand() % 3 + 1;
      }
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
      break;
    case 1:
      if (list_9c_.op == 0) {
        if (IsSideBeaten(unused, 0)) {
          return 1;
        }
        if (--list_218_.arg <= 0) {
          return 1;
        }
        if (rand() & 7) {
          list_9c_.Start(0x10002, 0);
        } else {
          list_9c_.Start(0x10005, 0);
        }
      }
      break;
  }
  return 0;
}

// 0x4162f0
int CEnmParam::Tactic4162f0(int unused) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      list_9c_.Start(0x10002, 0);
      if (IsSideBeaten(unused, 50)) {
        list_b0_.Start(0x20001, 0);
      }
      ++list_218_.step;
      break;
    case 1:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      if (list_9c_.op == 0) {
        list_9c_.Start(0x10002, 0);
      }
      break;
  }
  return 0;
}

// 0x416560
int CEnmParam::Tactic416560(int unused) {
  RunOpening(unused, 10);
  switch (list_218_.step) {
    case 10:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      list_9c_.Start(0x10002, 0);
      list_b0_.Start(0x20002, 0);
      ++list_218_.step;
      break;
    case 11:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x416630
// TODO: nonmatching (VC loads, increments and stores the step where the
// original increments it in memory)
int CEnmParam::Tactic416630(int unused) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 50);
      lists_c4_[1].op = 0;
      list_218_.arg = rand() % 3 + 3;
      ++list_218_.step;
      // Fall through.
    case 1:
      if (core_power_[1][4] == 0) {
        return 1;
      }
      list_b0_.Start(0x20004, 0x200);
      ++list_218_.step;
      break;
    case 2:
      if (list_b0_.op == 0 || (state_.flags & 2)) {
        lists_c4_[1].Start(0x30004, 4);
        list_218_.time = 60;
        ++list_218_.step;
      }
      break;
    case 3:
      if (list_218_.time <= 0 && list_b0_.op == 0) {
        if (--list_218_.arg <= 0) {
          list_b0_.Start(0x20002, 0);
          ++list_218_.step;
        } else {
          list_218_.step = 1;
        }
      }
      break;
    case 4:
      if (list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4167a0
int CEnmParam::Tactic4167a0(int unused) {
  switch (list_218_.step) {
    case 0:
      if (core_power_[0][6] <= 200 && core_power_[1][6] <= 200) {
        return 1;
      }
      lists_c4_[0].Start(0x30005, 6);
      lists_c4_[1].Start(0x30005, 6);
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
      // Fall through.
    case 1:
      if (list_9c_.op == 0) {
        if (lists_c4_[0].op == 0 && lists_c4_[1].op == 0) {
          return 1;
        }
        list_9c_.Start(0x10002, 0);
      }
      if (core_power_[0][6] <= 0 && core_power_[1][6] <= 0) {
        lists_c4_[0].Start(0x30001, 100);
        lists_c4_[1].Start(0x30001, 100);
        ++list_218_.step;
      }
      break;
    case 2:
      if (list_9c_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4168e0
int CEnmParam::Tactic4168e0(int unused) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 200);
      lists_c4_[1].Start(0x30001, 200);
      list_218_.time = (rand() % 10 + 5) * 60;
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
      // Fall through.
    case 1:
      if (list_9c_.op == 0) {
        if (core_count_ < 5 || list_218_.time <= 0) {
          return 1;
        }
        list_9c_.Start(0x10002, 0);
      }
      break;
  }
  return 0;
}

// 0x4169c0
int CEnmParam::Tactic4169c0(int unused) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      list_b0_.Start(0x20003, (rand() % 20 + 20) * 60);
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
      break;
    case 1:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      if (list_9c_.op == 0) {
        list_9c_.Start(0x10002, 0);
      }
      break;
  }
  return 0;
}

// 0x416ac0
// TODO: nonmatching (case 12 stores the step before the time; the original
// stores the time first)
int CEnmParam::Tactic416ac0(int unused) {
  RunOpening(unused, 10);
  switch (list_218_.step) {
    case 10:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].op = 0;
      list_218_.arg = rand() % 3 + 3;
      // Fall through.
    case 11:
      if (list_218_.arg > 0 && core_power_[1][4] > 0) {
        list_b0_.Start(0x20004, 0x200);
        ++list_218_.step;
      } else {
        list_218_.step = 20;
      }
      break;
    case 12:
      if (list_b0_.op == 0 || (state_.flags & 2)) {
        lists_c4_[1].Start(0x30004, 4);
        list_218_.time = 60;
        ++list_218_.step;
      }
      break;
    case 13:
      if (list_218_.time <= 0 && list_b0_.op == 0) {
        --list_218_.arg;
        list_218_.step = 11;
      }
      break;
    case 20:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      list_9c_.Start(0x10002, 0);
      list_b0_.Start(0x20002, 0);
      ++list_218_.step;
      // Fall through.
    case 21:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x416cb0
int CEnmParam::Tactic416cb0(int unused) {
  RunOpening(unused, 10);
  switch (list_218_.step) {
    case 10:
      lists_c4_[0].Start(0x30001, 200);
      lists_c4_[1].Start(0x30001, 200);
      list_b0_.Start(0x20001, 0);
      ++list_218_.step;
      // Fall through.
    case 11:
      if (list_b0_.op == 0) {
        list_218_.step = 20;
      }
      break;
    case 20:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      list_9c_.Start(0x10002, 0);
      list_b0_.Start(0x20002, 0);
      ++list_218_.step;
      // Fall through.
    case 21:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}
