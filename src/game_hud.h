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
    kFlagNameCursor = 0x1,    // the name entry cursor is shown
    kFlagSkip = 0x100,        // a button was pressed during the screen
    kFlagScore = 0x10000,     // "1P:" and the score
    kFlagTimer = 0x20000,     // the clock
    kFlagLives = 0x40000,     // "LEFT:" and the extend counter
    kFlagReplay = 0x1000000,  // the name and date of the replay played
  };
  // texts_.
  enum Texts {
    kTextScore,
    kTextLives,
    kTextExtend,
    kTextSeconds,
    kTextFrames,
    kTextReplayName,
    kTextReplayDate,
    kTextReplayHelp,
  };

  GameHud();  // 0x41f010

  // Starts the screen `mode` with the next IsProtected().
  void RequestMode(int mode) { next_mode_ = mode; }
  // Nonzero while a screen runs or is about to.
  int busy() const { return next_mode_ | mode_; }
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

  // Draw the current screen.
  void DrawGameOver();   // 0x420500
  void DrawNameEntry();  // 0x41f830
  void DrawResult();     // 0x4200e0

  // Puts the boss chart at the origin; always 1.
  int InitChart();  // 0x420290
  // Set up the name entry and result screens; always 1 (folded by
  // /OPT:ICF into 0x410340).
  int InitNameEntryScreen();
  int InitResultScreen();
  void InitGameOver();     // 0x4202e0
  void ReleaseGameOver();  // 0x4202f0
  // Places the corners of the boss chart from shown_params_.
  void UpdateChart();  // 0x420330
  // Reads the boss's parameters and starts the chart from the first one.
  void LoadParams();        // 0x4204b0
  int MoveGameOver();       // 0x420bd0
  void InitNameEntry();     // 0x41f5a0
  void ReleaseNameEntry();  // 0x41f5b0
  // Moves the name entry cursor by the pad; 1 once the name is ended.
  int EnterName();       // 0x41f600
  int MoveNameEntry();   // 0x41fc90
  void InitResult();     // 0x41fef0
  void ReleaseResult();  // 0x41ff00
  int MoveResult();      // 0x41ff20

  union {
    unsigned int flags_;        // 0x14
    unsigned char mode_flags_;  // 0x14, the low byte, cleared by SetMode
  };
  int mode_;           // 0x18
  int next_mode_;      // 0x1c, started by the next IsProtected()
  int step_;           // 0x20, of the current screen
  int mode_time_;      // 0x24, frames in mode_
  int replay_name_x_;  // 0x28, right-aligned at column 80
  HLSTxt* texts_[8];   // 0x2c
  // The state of the current screen.
  union {
    HLSTxt* mode_texts_[4];  // 0x4c, the texts of the screen
    // The name entry screen.
    struct {
      HLSTxt* name_entry_texts_[4];  // 0x4c
      int cursor_;                   // 0x5c, the letter being entered (0..3)
      int char_index_;    // 0x60, the character selected in kNameChars
      float back_alpha_;  // 0x64, of the backdrop
      float text_alpha_;  // 0x68
      float open_;        // 0x6c, how far the window is open (0..1)
      char name_[4];      // 0x70
      short last_dirs_;   // 0x74, the directions held last frame
      short dirs_time_;   // 0x76, frames they have been held
    };
    // The boss result screen.
    struct {
      HLSTxt* result_text_;  // 0x4c
      float result_bar_;     // 0x50, how far the bar is shown (0..1)
      float result_alpha_;   // 0x54, of the text
      int result_height_;    // 0x58, of the text in pixels
      short result_shown_;   // 0x5c, characters typed out so far
      short result_length_;  // 0x5e, of the text
    };
    // The game over screen.
    struct {
      HLSTxt* game_over_texts_[3];  // 0x4c, "NAME:", "STAGE:", the title
      // The boss's title: measured, then where it is drawn.
      union {
        SIZE title_size_;  // 0x58
        POINT title_pos_;  // 0x58
      };
      float title_slide_;  // 0x60, of the title from the right
      float frame_open_;   // 0x64, how far the frame is open (0..1)
      float band_height_;  // 0x68
      float chart_alpha_;  // 0x6c
      float info_alpha_;   // 0x70
      // The chart of the boss's parameters.
      int param_count_;         // 0x74
      short params_[13];        // 0x78, CEnmParam::GetParams(), at least 0x40
      short shown_params_[13];  // 0x92, the values drawn so far
      D3DVECTOR chart_center_;  // 0xac
      D3DVECTOR chart_[7];      // 0xb8, the corners, closed
    };
  };
};

// The corners of the unit hexagon, closed (the last is the first).
extern D3DVECTOR g_hexagon[7];  // 0x45d0a0

#endif  // WF_GAME_HUD_H_
