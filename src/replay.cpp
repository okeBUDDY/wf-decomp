// Replay: recording and playing back the player's input.
#include "replay.h"

#include <stdlib.h>
#include <string.h>

#include "actor_control.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "game.h"
#include "hl_input.h"

// 0x415540
int Replay::LoadPlayer(Actor* player) {
  if (mode_ != kPlay) {
    return 1;
  }
  if (!file_.IsOpen()) {
    return 0;
  }
  if (!Control01::Load(player, &file_)) {
    file_.Close();
    return 0;
  }
  return 1;
}

// 0x415590
void Replay::Close() {
  if (flags_ & kOpen) {
    switch (mode_) {
      case kRecord:
        file_.Write(&run_, sizeof(run_));
        break;
    }
  }
  file_.Close();
  flags_ &= ~kOpen;
}

// 0x4155c0
void Replay::GetInput(unsigned short* keys, unsigned short* pressed) {
  HLInpDev* pad = g_application.input->pad(0);
  unsigned short held = pad->buttons();
  unsigned short down = pad->pressed();
  switch (mode_) {
    case kRecord:
      if (flags_ & kOpen) {
        if (frame_ == 0) {
          if (!file_.WriteExact(&down, 2)) {
            flags_ &= ~kOpen;
          }
          run_.keys = held;
          run_.frames = 1;
        } else {
          if (last_keys_ != held) {
            if (!file_.WriteExact(&run_, sizeof(run_))) {
              flags_ &= ~kOpen;
            }
            run_.keys = held;
            run_.frames = 1;
          } else {
            ++run_.frames;
          }
          down = ~last_keys_ & held;
        }
        ++frame_;
      }
      break;
    case kPlay:
      down = 0;
      held = 0;
      if (flags_ & kOpen) {
        if (frame_ == 0) {
          if (!file_.ReadExact(&down, 2)) {
            flags_ &= ~kOpen;
          }
          if (!file_.ReadExact(&run_, sizeof(run_))) {
            flags_ &= ~kOpen;
          }
          held = run_.keys;
          --run_.frames;
        } else {
          if (run_.frames <= 0) {
            if (!file_.ReadExact(&run_, sizeof(run_))) {
              flags_ &= ~kOpen;
            }
          }
          held = run_.keys;
          down = ~last_keys_ & held;
          --run_.frames;
        }
        ++frame_;
      }
      break;
  }
  *keys = held;
  *pressed = down;
  last_keys_ = held;
}

// 0x415720
int Replay::Open(int mode, const char* path) {
  Close();
  strcpy(path_, path);
  mode_ = mode;
  flags_ = 0;
  return 1;
}

// 0x415790
// TODO: nonmatching. Closer in shape (but more bytes off): copy
// header.rules field by field, and read header.stage/score/timer into
// locals before g_game.Start() (the original keeps them in edi/ebx/ebp across
// the calls); ebx/ebp are then still pushed later than in the original.
int Replay::LoadHeader() {
  if (mode_ != kPlay) {
    return 1;
  }
  if (!file_.Open(path_, File::kRead)) {
    return 0;
  }
  int version;
  if (!file_.ReadExact(&version, sizeof(version))) {
    file_.Close();
    return 0;
  }
  GameRules rules;
  g_game.GetDefaultRules(&rules);
  Header header;
  int lives;
  int extend_count;
  switch (version) {
    case kOldVersion:
      if (!file_.ReadExact(&header, 0x10)) {
        file_.Close();
        return 0;
      }
      seed_ = header.seed;
      lives = -1;
      extend_count = -1;
      break;
    case kVersion:
      if (!file_.ReadExact(&header, sizeof(header))) {
        file_.Close();
        return 0;
      }
      seed_ = header.seed;
      lives = header.lives;
      extend_count = header.extend_count;
      rules = header.rules;
      break;
    default:
      file_.Close();
      return 0;
  }
  g_game.Start(&rules);
  g_game.timer_ = header.timer;
  g_game.bonus_ = 0;
  g_game.SetLives(lives + 1, extend_count);
  g_global.entry.score = header.score;
  g_global.stage = header.stage - 1;
  if (!g_enm_param.Load(&file_)) {
    file_.Close();
    return 0;
  }
  return 1;
}

// 0x415910
void Replay::Start(Actor* player) {
  switch (mode_) {
    case kRecord: {
      Close();
      seed_ = rand();
      if (!file_.Open(path_, File::kWrite)) {
        return;
      }
      int version = kVersion;
      if (!file_.WriteExact(&version, sizeof(version))) {
        file_.Close();
        return;
      }
      Header header;
      header.seed = seed_;
      header.stage = g_global.stage;
      header.score = g_global.entry.score;
      header.timer = g_game.timer_ + g_game.bonus_;
      header.lives = g_game.lives_;
      header.extend_count = g_game.extend_count_;
      header.rules.mode = g_game.rules_.mode;
      header.rules.time_limit = g_game.rules_.time_limit;
      header.rules.stock = g_game.rules_.stock;
      header.rules.extend = g_game.rules_.extend;
      if (!file_.WriteExact(&header, sizeof(header))) {
        file_.Close();
        return;
      }
      if (!g_enm_param.Save(&file_)) {
        file_.Close();
        return;
      }
      if (!Control01::Save(player, &file_)) {
        file_.Close();
        return;
      }
    }
    // Fall through.
    case kPlay:
      srand(seed_);
      frame_ = 0;
      flags_ |= kOpen;
      run_.frames = 0;
      break;
  }
}

// 0x415a90
Replay::Replay() {
  mode_ = kNone;
  flags_ = 0;
}

// 0x415ac0
int Replay::IsReplayFile(const char* path) {
  File file;
  if (!file.Open(path, File::kRead)) {
    return 0;
  }
  unsigned int version;
  if (file.ReadExact(&version, sizeof(version)) && version >= kOldVersion &&
      version <= kVersion) {
    return 1;
  }
  return 0;
}
