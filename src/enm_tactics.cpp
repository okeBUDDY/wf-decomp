// The battle tactics of CEnmParam (list_218_).
#include <stdlib.h>

#include "enm_param.h"

// 0x4161d0
int CEnmParam::Tactic4161d0(Actor* boss) {
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
        if (IsSideBeaten(boss, 0)) {
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
int CEnmParam::Tactic4162f0(Actor* boss) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      list_9c_.Start(0x10002, 0);
      if (IsSideBeaten(boss, 50)) {
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
int CEnmParam::Tactic416560(Actor* boss) {
  RunOpening(boss, 10);
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
int CEnmParam::Tactic416630(Actor* boss) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 50);
      lists_c4_[1].op = 0;
      list_218_.SetArgAndNext(rand() % 3 + 3);
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
        list_218_.Wait(60);
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
int CEnmParam::Tactic4167a0(Actor* boss) {
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
int CEnmParam::Tactic4168e0(Actor* boss) {
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
int CEnmParam::Tactic4169c0(Actor* boss) {
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
int CEnmParam::Tactic416ac0(Actor* boss) {
  RunOpening(boss, 10);
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
        list_218_.Wait(60);
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
int CEnmParam::Tactic416cb0(Actor* boss) {
  RunOpening(boss, 10);
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

// 0x416e10
int CEnmParam::Tactic416e10(Actor* boss) {
  switch (list_218_.step) {
    case 0:
      list_218_.arg = rand() % 3 + 3;
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      list_b0_.Start(0x20004, 0);
      ++list_218_.step;
      // Fall through.
    case 1:
      if (list_b0_.op == 0 || (state_.flags & 2)) {
        lists_c4_[0].op = 0;
        lists_c4_[1].op = 0;
        list_9c_.Start(0x10003, 0);
        ++list_218_.step;
      }
      break;
    case 2:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        list_218_.Wait(60);
      }
      break;
    case 3:
      if (list_218_.time <= 0) {
        --list_218_.arg;
        lists_c4_[0].Start(0x30001, 100);
        lists_c4_[1].Start(0x30001, 100);
        if (list_218_.arg <= 0) {
          list_9c_.Start(0x10002, 0);
          list_b0_.Start(0x20002, 0);
          ++list_218_.step;
        } else {
          list_b0_.Start(0x20004, 0);
          list_218_.step = 1;
        }
      }
      break;
    case 4:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x417010
int CEnmParam::Tactic417010(Actor* boss) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 200);
      lists_c4_[1].Start(0x30001, 200);
      list_b0_.Start(0x20003, (rand() % 20 + 20) * 60);
      ++list_218_.step;
      break;
    case 1:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      break;
  }
  return 0;
}

// 0x4170c0
int CEnmParam::Tactic4170c0(Actor* boss) {
  switch (list_218_.step) {
    case 0:
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      // Fall through.
    case 1:
      if (list_b0_.op == 0) {
        list_b0_.Start(0x20002, 0);
        ++list_218_.step;
      }
      break;
  }
  if (list_9c_.op == 0) {
    list_9c_.Start(0x10002, 300);
  }
  return 0;
}

// 0x417430
int CEnmParam::Tactic417430(Actor* boss) {
  switch (list_218_.step) {
    case 0: {
      int time = (rand() % 5 + 5) * 60;
      if (!GlowArmor(boss, time)) {
        return 1;
      }
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      list_218_.time = time;
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
    }
      // Fall through.
    case 1:
      if (list_218_.time <= 0) {
        if (list_9c_.op == 0) {
          return 1;
        }
      } else if (list_9c_.op == 0) {
        list_9c_.Start(0x10002, 0);
      }
      break;
  }
  return 0;
}

// 0x417520
int CEnmParam::Tactic417520(Actor* boss) {
  switch (list_218_.step) {
    case 2:
      if (list_218_.time <= 0) {
        return 1;
      }
      break;
    case 1:
      if (trans_count_ <= 0) {
        list_218_.Wait(90);
      }
      break;
    case 0:
      if (!SignalUnknown104(boss)) {
        return 1;
      }
      lists_c4_[0].Start(0x30001, 70);
      lists_c4_[1].Start(0x30001, 70);
      ++list_218_.step;
      break;
  }
  return 0;
}

// 0x4175d0
int CEnmParam::Tactic4175d0(Actor* boss) {
  switch (list_218_.step) {
    case 0: {
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].op = 0;
      int time = (rand() % 20 + 20) * 60;
      GlowArmor(boss, time);
      list_b0_.Start(0x20003, time);
      list_218_.Wait(60);
      break;
    }
    case 1:
      if (list_b0_.op == 0) {
        return 1;
      }
      if (list_218_.time <= 0) {
        if (core_power_[1][4] > 0) {
          lists_c4_[1].Start(0x30004, 4);
          list_218_.time = 60;
        } else {
          lists_c4_[1].Start(0x30001, 100);
          list_218_.time = 60;
        }
      }
      break;
  }
  return 0;
}

// 0x4176f0
int CEnmParam::Tactic4176f0(Actor* boss) {
  switch (list_218_.step) {
    case 0: {
      lists_c4_[0].Start(0x30001, 200);
      lists_c4_[1].Start(0x30001, 200);
      int time = (rand() % 10 + 5) * 60;
      GlowArmor(boss, time);
      list_218_.time = time;
      list_9c_.Start(0x10002, 0);
      ++list_218_.step;
    }
      // Fall through.
    case 1:
      if (list_218_.time <= 0) {
        if (list_9c_.op == 0) {
          return 1;
        }
      } else if (list_9c_.op == 0) {
        list_9c_.Start(0x10002, 0);
      }
      break;
  }
  return 0;
}

// 0x4177e0
int CEnmParam::Tactic4177e0(Actor* boss) {
  switch (list_218_.step) {
    case 0: {
      lists_c4_[0].Start(0x30001, 100);
      lists_c4_[1].Start(0x30001, 100);
      int time = (rand() % 20 + 20) * 60;
      GlowArmor(boss, time);
      list_b0_.Start(0x20003, time);
      list_9c_.Start(0x10005, 0);
      ++list_218_.step;
      break;
    }
    case 1:
      if (list_9c_.op == 0 && list_b0_.op == 0) {
        return 1;
      }
      if (list_9c_.op == 0) {
        list_9c_.Start(0x10005, 0);
      }
      break;
  }
  return 0;
}
