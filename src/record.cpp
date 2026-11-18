// The records (replays) of cGlobal and the rating of a finished game.
#include <windows.h>

#include "cglobal.h"
#include "enm_param.h"

GameRating g_rating;

// 0x4186f0
int cGlobal::InitRecords() {
  replay_path[0] = 0;
  last_record[0] = 0;
  CreateDirectoryA("rec\\", NULL);
  return 1;
}

// 0x418710
void cGlobal::ClearReplay() { replay_path[0] = 0; }

// Rates the game ending at the current stage.
static inline void SetRating(int grade, int title) {
  g_rating.stage = g_global.stage;
  g_rating.grade = grade;
  g_rating.title = title;
}

// 0x418b60
// TODO: nonmatching (the original merges the stores into the tail of the
// first case, with the values in registers).
void RateGame() {
  const EnmSideStats* sides = g_enm_param.state_.sides;
  if (sides[0].mode == 0 && sides[1].mode == 0) {
    SetRating(0, 0);
  } else if (sides[0].mode == 0 || sides[1].mode == 0) {
    // Only one side reached a later mode.
    SetRating(1, 0);
  } else if (sides[0].mode == 3 || sides[1].mode == 3) {
    if (sides[0].progress >= 100 && sides[1].progress >= 100) {
      SetRating(6, g_enm_param.state_.title.words[0] - 13);
    } else {
      SetRating(5, g_enm_param.state_.title.words[0] - 13);
    }
  } else if (sides[0].progress >= 100 && sides[1].progress >= 100) {
    SetRating(5, g_enm_param.state_.title.words[0] - 13);
  } else if (sides[0].progress >= 65) {
    SetRating((sides[1].progress >= 65) + 3, 0);
  } else {
    SetRating((sides[1].progress >= 65) + 2, 0);
  }
}
