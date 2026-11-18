// GameHud: the in-game display task of Application::overlays (vtable
// 0x453494), FnGame's second task. Besides the status texts it runs one
// screen at a time: the game over screen, name entry and the boss result.
// Only the fields used by decompiled code are known so far.
#ifndef WF_GAME_HUD_H_
#define WF_GAME_HUD_H_

#include <windows.h>

#include <d3d.h>

#include "task_list.h"

class HLSTxt;

class GameHud : public Task {
 public:
  enum Mode {
    kModeNone = 0,
    kModeGameOver = 1,   // "NAME:" / "STAGE:"
    kModeNameEntry = 2,  // "PLEASE ENTER YOUR NAME!"
    kModeResult = 3,     // the boss result (CEnmParam::FormatResult)
  };
  enum Flags {
    kFlagSkip = 0x100,  // a button was pressed during the screen
  };

  GameHud();                 // 0x41f010
  virtual int Initialize();  // 0x41f060
  virtual void Finalize();   // 0x41f280
  // Runs the current screen and ends it when it is done; always 1.
  virtual int IsProtected();  // 0x41ef80
  virtual void Run();         // 0x41f340

 private:
  // Ends the current screen and starts `mode`.
  void SetMode(int mode);  // 0x41eef0
  // Moves *`out` from `from` to `to` over `frames`; 1 once `time` reaches
  // `frames`.
  int Lerp(float* out, float from, float to, int frames, int time);  // 0x41f560
  int Lerp(int* out, int from, int to, int frames, int time);        // 0x420250

  // Puts the boss chart at the origin; always 1.
  int InitChart();         // 0x420290
  void InitGameOver();     // 0x4202e0
  void ReleaseGameOver();  // 0x4202f0
  // Places the corners of the boss chart from shown_params_.
  void UpdateChart();  // 0x420330
  // Reads the boss's parameters and starts the chart from the first one.
  void LoadParams();        // 0x4204b0
  int MoveGameOver();       // 0x420bd0
  void InitNameEntry();     // 0x41f5a0
  void ReleaseNameEntry();  // 0x41f5b0
  int MoveNameEntry();      // 0x41fc90
  void InitResult();        // 0x41fef0
  void ReleaseResult();     // 0x41ff00
  int MoveResult();         // 0x41ff20

  union {
    unsigned int flags_;        // 0x14
    unsigned char mode_flags_;  // 0x14, the low byte, cleared by SetMode
  };
  int mode_;        // 0x18
  int next_mode_;   // 0x1c, started by the next IsProtected()
  int unknown_20_;  // 0x20
  int mode_time_;   // 0x24, frames in mode_
  char unknown_28_[0x2c - 0x28];
  HLSTxt* texts_[8];       // 0x2c
  HLSTxt* mode_texts_[4];  // 0x4c, the texts of the current screen
  char unknown_5c_[0x74 - 0x5c];
  // The game over screen's chart of the boss's parameters.
  int param_count_;         // 0x74
  short params_[13];        // 0x78, CEnmParam::GetParams(), at least 0x40
  short shown_params_[13];  // 0x92, the values drawn so far
  D3DVECTOR chart_center_;  // 0xac
  D3DVECTOR chart_[7];      // 0xb8, the corners, closed
};

// The corners of the unit hexagon, closed (the last is the first).
extern D3DVECTOR g_hexagon[7];  // 0x45d0a0

#endif  // WF_GAME_HUD_H_
