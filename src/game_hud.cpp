// GameHud: the in-game display. Each screen is a unit of its own
// (game_hud_*.cpp), which keeps its functions out of line here.
#include "game_hud.h"

#include "fn_game.h"
#include "hls_txt.h"

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
  unknown_20_ = 0;
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
  unknown_20_ = 0;
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
