// The records (replays) of cGlobal and the rating of a finished game.
#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "cglobal.h"
#include "enm_param.h"
#include "fn_game.h"
#include "replay.h"
#include "score_table.h"

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

// 0x418720
// TODO: nonmatching (the original shares one `return 1` epilogue, which VC
// duplicates into the last-record branch here)
int cGlobal::NextReplay(char* path) {
  DeleteFileA("rec\\temp.rec");
  int found = 0;
  if (last_record[0] != 0) {
    strcpy(path, last_record);
    last_record[0] = 0;
    found = g_replay.IsReplayFile(path);
  }
  if (found) {
    strcpy(replay_path, path);
  } else {
    char next[0x48] = "", first[0x48] = "";
    WIN32_FIND_DATAA find;
    HANDLE handle = FindFirstFileA("rec\\*.rec", &find);
    if (handle == INVALID_HANDLE_VALUE) {
      return 0;
    }
    char file[0x48];
    strcpy(file, "rec\\");
    char* name = file + strlen(file);
    do {
      if (strlen(find.cFileName) >= 0x40) {
        continue;
      }
      strcpy(name, find.cFileName);
      if (!g_replay.IsReplayFile(file)) {
        continue;
      }
      if (first[0] == 0 || strcmp(first, file) > 0) {
        strcpy(first, file);
      }
      if (replay_path[0] == 0 || strcmp(replay_path, file) < 0) {
        if (next[0] == 0 || strcmp(next, file) >= 0) {
          strcpy(next, file);
        }
      }
    } while (FindNextFileA(handle, &find));
    FindClose(handle);
    if (next[0] != 0) {
      strcpy(path, next);
    } else if (first[0] != 0) {
      strcpy(path, first);
    } else {
      return 0;
    }
    strcpy(replay_path, path);
  }
  return 1;
}

// 0x418a00
void cGlobal::SaveReplay(int finished) {
  char path[0x48] = "";
  const EnmTitle& title = g_enm_param.state_.title;
  if (title.weapon == -1 && title.variant == -1 && title.words[1] == -1 &&
      title.words[0] >= 14) {
    sprintf(path, "rec\\boss%02d.rec", title.words[0] - 14);
    CopyFileA("rec\\temp.rec", path, FALSE);
  }
  if (finished) {
    if (g_score_table.rank() == 0) {
      strcpy(path, "rec\\best.rec");
      CopyFileA("rec\\temp.rec", path, FALSE);
    }
    strcpy(last_record, "rec\\last.rec");
    CopyFileA("rec\\temp.rec", last_record, FALSE);
  }
  if (g_global.flags & cGlobal::kFlagStageReplays) {
    sprintf(path, "rec\\stage%02d.rec", stage);
    CopyFileA("rec\\temp.rec", path, FALSE);
    strcpy(last_record, "rec\\stage01.rec");
  }
  DeleteFileA("rec\\temp.rec");
}

// Rates the game ending at the current stage.
static inline void SetRating(int grade, int title) {
  g_rating.stage = g_global.stage;
  g_rating.grade = grade;
  g_rating.title = title;
}

// 0x418b60
// TODO: nonmatching (the original merges the stores into the tail of the
// first case, with the values in registers).
void FnGame::RateGame() {
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
