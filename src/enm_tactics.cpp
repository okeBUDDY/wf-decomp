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
