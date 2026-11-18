// fnGame: the stage that runs a game (vtable 0x4530a8, g_fn_game).
// Only the fields used by decompiled code are known so far.
#ifndef WF_FN_GAME_H_
#define WF_FN_GAME_H_

#include "hl_input.h"
#include "user_func.h"

class Task;

class FnGame : public UserFunc {
 public:
  // The player's buttons in keys_ and pressed_.
  enum Keys {
    kKeyDirections =
        HLInpDev::kUp | HLInpDev::kDown | HLInpDev::kLeft | HLInpDev::kRight,
    kKeyFire = HLInpDev::kTrigger << 0,
    kKeySlow = HLInpDev::kTrigger << 1,
    kKeySwitch = HLInpDev::kTrigger << 3,  // the weapon
  };

  FnGame();                  // 0x41be00
  virtual int Initialize();  // 0x41bf50
  virtual void Finalize();   // 0x41c0e0
  virtual void Move();       // 0x41c190
  virtual void Draw();       // 0x41bea0

  // Starts recording (`mode` 0) or playing back `path` (1, 2).
  void SetMode(int mode, const char* path);  // 0x41be30
  // Enters `state`.
  void SetState(short state);  // 0x41be80

  unsigned short keys() const { return keys_; }
  unsigned short pressed() const { return pressed_; }

 private:
  Task* unknown_14_;        // 0x14
  Task* unknown_18_;        // 0x18
  unsigned int flags_;      // 0x1c
  int mode_;                // 0x20
  int unknown_24_;          // 0x24
  short unknown_28_;        // 0x28
  short unknown_2a_;        // 0x2a
  short state_;             // 0x2c
  short state_time_;        // 0x2e, frames in state_
  int unknown_30_;          // 0x30
  unsigned short keys_;     // 0x34, held this frame
  unsigned short pressed_;  // 0x36, pressed this frame
};

extern FnGame g_fn_game;  // 0x467c98

#endif  // WF_FN_GAME_H_
