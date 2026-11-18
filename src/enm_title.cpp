// The title of the planned boss.
#include <stdlib.h>
#include <string.h>

#include "enm_param.h"

// 0x417c10
void CEnmParam::ChooseTitle() {
  EnmTitle title;
  memset(&title, 0xff, sizeof(title));
  if (state_.sides[0].mode == 0 && state_.sides[1].mode == 0) {
    title.words[0] = 0;
  } else if (state_.sides[0].mode == 3 || state_.sides[1].mode == 3) {
    title.words[0] = state_.title.words[0];
    if (state_.sides[0].mode == 4) {
      if (state_.sides[1].progress >= 100) {
        title.words[0] = 0x17;
      }
    } else if (state_.sides[1].mode == 4) {
      if (state_.sides[0].progress >= 100) {
        title.words[0] = 0x18;
      }
    } else if (state_.sides[0].progress >= 100 &&
               state_.sides[1].progress >= 100) {
      title.words[0] = 0x19;
    }
  } else if (state_.sides[0].progress >= 100 &&
             state_.sides[1].progress >= 100) {
    switch (state_.sides[0].mode) {
      case 1:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0xe;
            break;
          case 2:
            title.words[0] = 0xf;
            break;
          case 4:
            title.words[0] = 0x10;
            break;
        }
        break;
      case 2:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0x11;
            break;
          case 2:
            title.words[0] = 0x12;
            break;
          case 4:
            title.words[0] = 0x13;
            break;
        }
        break;
      case 4:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0x14;
            break;
          case 2:
            title.words[0] = 0x15;
            break;
          case 4:
            title.words[0] = 0x16;
            break;
        }
        break;
    }
  } else {
    if (state_.sides[0].progress >= 0x41) {
      switch (state_.sides[0].mode) {
        case 1:
          title.words[1] = 0xb;
          break;
        case 2:
          title.words[1] = 0xc;
          break;
        case 4:
          title.words[1] = 0xd;
          break;
      }
    } else if (state_.sides[0].mode != 0) {
      switch (state_.sides[0].mode) {
        case 1:
          title.words[1] = 4;
          break;
        case 2:
          title.words[1] = 5;
          break;
        case 4:
          title.words[1] = 6;
          break;
      }
    }
    if (state_.sides[1].progress >= 0x41) {
      switch (state_.sides[1].mode) {
        case 1:
          title.words[0] = 8;
          break;
        case 2:
          title.words[0] = 9;
          break;
        case 4:
          title.words[0] = 0xa;
          break;
      }
    } else if (state_.sides[1].mode != 0) {
      if (state_.sides[0].mode == 0) {
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 4;
            break;
          case 2:
            title.words[0] = 5;
            break;
          case 4:
            title.words[0] = 6;
            break;
        }
      } else if (state_.sides[1].mode == state_.sides[0].mode &&
                 state_.sides[0].progress < 0x41) {
        title.words[0] = 7;
      } else {
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 1;
            break;
          case 2:
            title.words[0] = 2;
            break;
          case 4:
            title.words[0] = 3;
            break;
        }
      }
    }
  }

  EnmTitle* seen = titles_;
  int count = title_count_;
  int i;
  for (i = 0; i < title_count_; ++i, ++seen) {
    if (seen->words[0] == title.words[0] && seen->words[1] == title.words[1]) {
      break;
    }
  }
  if (i != count) {
    if (new_weapon_ >= 0 && weapon_parts_[new_weapon_] != 0) {
      title.weapon = new_weapon_;
      title.level = weapon_parts_[new_weapon_] * 2;
    } else {
      int most = -1;
      int main;
      int kinds = 0;
      int j;
      for (j = 0; j < 7; ++j) {
        if (weapon_parts_[j] > 0) {
          if (weapon_parts_[j] > most) {
            most = weapon_parts_[j];
            main = j;
          }
          ++kinds;
        }
      }
      for (j = 0; j < 7; ++j) {
        if (j != main && most - weapon_parts_[j] < 3) {
          break;
        }
      }
      if (j == 7) {
        title.weapon = main;
        title.level = most * 2;
      } else {
        title.weapon = 7;
        title.level = kinds;
      }
    }
    if (title.level > 20) {
      title.level = 20;
    }
    seen = titles_;
    for (i = 0; i < title_count_; ++i, ++seen) {
      if (seen->words[0] == title.words[0] &&
          seen->words[1] == title.words[1] && seen->weapon == title.weapon &&
          seen->level == title.level) {
        break;
      }
    }
  }
  if (i == count) {
    if (count >= 32) {
      title.variant = 20;
    } else {
      titles_[count] = title;
      ++title_count_;
    }
  } else {
    if (seen->variant >= 0) {
      if (++seen->variant >= 20) {
        seen->variant = 0;
      }
    } else {
      seen->variant = rand() % 20;
    }
    title.variant = seen->variant;
  }
  state_.title = title;
}
