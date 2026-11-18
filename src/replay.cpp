// Replay: recording and playing back the player's input.
#include "replay.h"

#include <string.h>

#include "actor_control.h"
#include "application.h"
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

// 0x415a90
Replay::Replay() {
  mode_ = kNone;
  flags_ = 0;
}
