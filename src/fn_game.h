// fnGame: the stage that runs a game (vtable 0x4530a8, g_fn_game).
// Only the fields used by decompiled code are known so far.
#ifndef WF_FN_GAME_H_
#define WF_FN_GAME_H_

#include "hl_input.h"
#include "user_func.h"

class GameHud;
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
  // Fades the screen to black in `fade` frames and ends the stage `wait`
  // frames after that (unless it is ending already).
  void Exit(int fade, int wait);  // 0x41c140
  // Tells the music the game started (g_seq_flags).
  void SetSeqFlags();  // 0x41c170
  // Rates the game that just ended (g_rating).
  void RateGame();  // 0x418b60

  // How the music goes from stage to stage (the BGM MODE option).
  enum BgmMode {
    kBgmLoop = 0,    // plays on
    kBgmFade = 1,    // fades out after each boss and in at the next stage
    kBgmRewind = 2,  // restarts at each stage
  };

  // 0 while recording, else playing back.
  int mode() const { return mode_; }
  void set_bgm_mode(int mode) { bgm_mode_ = mode; }
  unsigned short keys() const { return keys_; }
  unsigned short pressed() const { return pressed_; }

 private:
  enum Flags {
    kFlagPlayback = 0x1,    // playing a replay back
    kFlagReplayList = 0x2,  // then the next replay of the list
    kFlagMusic = 0x4,       // the music plays
  };
  // state_.
  enum State {
    kStateStart = 10,     // 10-11: the player appears
    kStateStage = 20,     // 20-21: the stage title
    kStateBattle = 30,    // 30-31: the boss fight
    kStateCleared = 40,   // 40-42: the boss is destroyed
    kStateGameOver = 50,  // 50-51
  };

  Task* unknown_14_;           // 0x14
  GameHud* hud_;               // 0x18
  unsigned int flags_;         // 0x1c
  int mode_;                   // 0x20
  int bgm_mode_;               // 0x24, BgmMode
  short exit_frames_;          // 0x28, frames until the stage ends, 0: running
  short exit_time_;            // 0x2a
  unsigned short state_;       // 0x2c
  unsigned short state_time_;  // 0x2e, frames in state_
  int stage_score_;            // 0x30, the score when the stage started
  unsigned short keys_;        // 0x34, held this frame
  unsigned short pressed_;     // 0x36, pressed this frame
};

extern FnGame g_fn_game;  // 0x467c98

#endif  // WF_FN_GAME_H_
