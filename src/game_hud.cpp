// GameHud: the in-game display. Each screen is a unit of its own
// (game_hud_*.cpp), which keeps its functions out of line here.
#include "game_hud.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "application.h"
#include "cglobal.h"
#include "fn_game.h"
#include "fonts.h"
#include "game.h"
#include "hl_3d.h"
#include "hls_txt.h"
#include "path_util.h"
#include "replay.h"

// The color of the status texts.
static const D3DCOLOR kTextColor = 0xc8ffc800;

// 0x41eef0
void GameHud::SetMode(int mode) {
  switch (mode_) {
    case kModeGameOver:
      ReleaseGameOver();
      break;
    case kModeNameEntry:
      ReleaseNameEntry();
      break;
    case kModeResult:
      ReleaseResult();
      break;
  }
  switch (mode) {
    case kModeGameOver:
      InitGameOver();
      break;
    case kModeNameEntry:
      InitNameEntry();
      break;
    case kModeResult:
      InitResult();
      break;
  }
  mode_ = mode;
  step_ = 0;
  mode_time_ = 0;
  mode_flags_ = 0;
}

// 0x41ef80
int GameHud::IsProtected() {
  if (next_mode_ != kModeNone) {
    SetMode(next_mode_);
    next_mode_ = kModeNone;
  }
  if (g_fn_game.pressed() &
      (FnGame::kKeyFire | FnGame::kKeySlow | FnGame::kKeySwitch)) {
    flags_ |= kFlagSkip;
  }
  switch (mode_) {
    case kModeGameOver:
      if (MoveGameOver()) {
        ReleaseGameOver();
        mode_ = kModeNone;
      }
      break;
    case kModeNameEntry:
      if (MoveNameEntry()) {
        ReleaseNameEntry();
        mode_ = kModeNone;
      }
      break;
    case kModeResult:
      if (MoveResult()) {
        ReleaseResult();
        mode_ = kModeNone;
      }
      break;
  }
  return 1;
}

// 0x41f010
GameHud::GameHud() {
  texts_[0] = NULL;
  texts_[1] = texts_[2] = NULL;
  texts_[3] = texts_[4] = NULL;
  texts_[5] = texts_[6] = texts_[7] = NULL;
  flags_ = 0;
  mode_ = next_mode_ = kModeNone;
  enabled_ = 1;
}

// 0x41f280
void GameHud::Finalize() {
  switch (mode_) {
    case kModeGameOver:
      ReleaseGameOver();
      break;
    case kModeNameEntry:
      ReleaseNameEntry();
      break;
    case kModeResult:
      ReleaseResult();
      break;
  }
  mode_ = kModeNone;
  step_ = 0;
  mode_time_ = 0;
  mode_flags_ = 0;
  if (texts_[0] != NULL) {
    texts_[0]->Release();
    texts_[0] = NULL;
  }
  if (texts_[1] != NULL) {
    texts_[1]->Release();
    texts_[1] = NULL;
  }
  if (texts_[2] != NULL) {
    texts_[2]->Release();
    texts_[2] = NULL;
  }
  if (texts_[3] != NULL) {
    texts_[3]->Release();
    texts_[3] = NULL;
  }
  if (texts_[4] != NULL) {
    texts_[4]->Release();
    texts_[4] = NULL;
  }
  if (texts_[5] != NULL) {
    texts_[5]->Release();
    texts_[5] = NULL;
  }
  if (texts_[6] != NULL) {
    texts_[6]->Release();
    texts_[6] = NULL;
  }
  if (texts_[7] != NULL) {
    texts_[7]->Release();
    texts_[7] = NULL;
  }
  flags_ = 0;
}

// 0x41f560
int GameHud::Lerp(float* out, float from, float to, int frames, int time) {
  if (time >= frames) {
    *out = to;
    return 1;
  }
  *out = (to - from) * time / frames + from;
  return 0;
}

// 0x41f060
int GameHud::Initialize() {
  flags_ = 0xff0000;
  next_mode_ = kModeNone;
  mode_ = kModeNone;
  texts_[kTextScore] = g_fonts.text.Create(0x20, NULL);
  texts_[kTextLives] = g_fonts.text.Create(8, NULL);
  texts_[kTextExtend] = g_fonts.small_text.Create(8, NULL);
  texts_[kTextSeconds] = g_fonts.large_digit.Create(8, NULL);
  texts_[kTextFrames] = g_fonts.digit.Create(8, NULL);
  if (texts_[kTextScore] == NULL || texts_[kTextSeconds] == NULL ||
      texts_[kTextFrames] == NULL) {
    goto error;
  }
  if (!InitChart() || !InitNameEntryScreen() || !InitResultScreen()) {
    return 0;
  }
  FILETIME time;
  if (g_replay.mode() == Replay::kPlay &&
      GetFileWriteTime(g_replay.path(), &time)) {
    texts_[kTextReplayName] = g_fonts.small_text.Create(0x48, NULL);
    texts_[kTextReplayDate] = g_fonts.small_text.Create(0x20, NULL);
    texts_[kTextReplayHelp] = g_fonts.small_text.Create(0x50, NULL);
    if (texts_[kTextReplayName] == NULL || texts_[kTextReplayDate] == NULL ||
        texts_[kTextReplayHelp] == NULL) {
      goto error;
    }
    char* name = texts_[kTextReplayName]->text();
    strcpy(name, GetFileName(g_replay.path()));
    replay_name_x_ = (80 - strlen(name)) * 8;
    for (; *name != '\0'; ++name) {
      *name = toupper(*name);
      if (*name < ' ' || *name > '_') {
        *name = '?';
      }
    }
    FILETIME local;
    SYSTEMTIME date;
    FileTimeToLocalFileTime(&time, &local);
    FileTimeToSystemTime(&local, &date);
    sprintf(texts_[kTextReplayDate]->text(), "%04d.%02d.%02d %02d:%02d",
            date.wYear, date.wMonth, date.wDay, date.wHour, date.wMinute);
    const char* help = "SHOT BUTTON -> NEXT REPLAY\nSLOW BUTTON -> GO TO TITLE";
    strcpy(texts_[kTextReplayHelp]->text(), help);
    flags_ |= kFlagReplay;
  }
  return 1;
error:
  Error("ﾃｷｽﾄｵﾌﾞｼﾞｪｸﾄが生成できません.");
  return 0;
}

// 0x41f340
void GameHud::Run() {
  g_application.d3d->SetAlphaBlend(D3DBLEND_INVSRCALPHA);
  switch (mode_) {
    case kModeGameOver:
      DrawGameOver();
      break;
    case kModeNameEntry:
      DrawNameEntry();
      break;
    case kModeResult:
      DrawResult();
      break;
  }
  if (flags_ & kFlagScore) {
    sprintf(texts_[kTextScore]->text(), "1P:%012d", g_global.entry.score);
    texts_[kTextScore]->Print(0, 0, kTextColor, -1);
  }
  if (flags_ & kFlagLives) {
    int lives = g_game.lives();
    int extend_count = g_game.extend_count();
    if (lives >= 0) {
      sprintf(texts_[kTextLives]->text(), "LEFT:%d", lives);
      texts_[kTextLives]->Print(0, 20, kTextColor, -1);
      if (extend_count >= 0) {
        sprintf(texts_[kTextExtend]->text(), ".%02d", extend_count);
        texts_[kTextExtend]->Print(96, 28, kTextColor, -1);
      }
    }
  }
  if (flags_ & kFlagTimer) {
    int seconds = g_game.timer() / 60;
    if (seconds >= 100) {
      sprintf(texts_[kTextSeconds]->text(), "%03d", seconds);
      texts_[kTextSeconds]->Print(448, 0, kTextColor, -1);
    } else {
      sprintf(texts_[kTextSeconds]->text(), "%02d", seconds);
      texts_[kTextSeconds]->Print(496, 0, kTextColor, -1);
    }
    sprintf(texts_[kTextFrames]->text(), "%02d", g_game.timer() % 60);
    texts_[kTextFrames]->Print(592, 1, kTextColor, -1);
  }
  if (flags_ & kFlagReplay) {
    texts_[kTextReplayName]->Print(replay_name_x_, 456, kTextColor, -1);
    texts_[kTextReplayDate]->Print(512, 468, kTextColor, -1);
    if (g_fn_game.mode() == 1) {
      texts_[kTextReplayHelp]->Print(0, 460, kTextColor, -1);
    }
  }
}
