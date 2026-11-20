// fnOption: the option screen (vtable 0x453354, g_fn_option); it also
// reads and writes the [Option] section of the profile. The original
// compiles it in one object with fnGame (fn_game.cpp).
#ifndef WF_FN_OPTION_H_
#define WF_FN_OPTION_H_

#include "game_rules.h"
#include "user_func.h"

class BackgroundGrid;
class HLSTxt;

class FnOption : public UserFunc {
 public:
  // The menu items, top to bottom.
  enum Item {
    kBossName,     // 0: Japanese, 1: English
    kBgmVolume,    // 0..9
    kSeVolume,     // 0..9
    kBgmMode,      // FnGame::BgmMode
    kScoreReset,   // 1: clear the scores of the current rules
    kStageReplay,  // 1: keep a replay of every stage
    kDialog,       // 1: show the setup dialog box at startup
    kGameMode,     // the preset of the next four
    kStartTime,    // index into the start times
    kTimerMode,    // GameRules::mode
    kShips,        // GameRules::stock
    kExtend,       // GameRules::Extend
    kShotButton,   // device codes of the triggers
    kSlowButton,
    kWeaponButton,
    kPause,  // 1: pause while the window is inactive
    kPauseButton,
    kExit,
    kItems
  };
  enum { kCustomPreset = 4 };

  FnOption();                // 0x41c6e0
  virtual int Initialize();  // 0x41dc60
  virtual void Finalize();   // 0x41ded0
  virtual void Move();       // 0x41d4d0
  virtual void Draw();       // 0x41d880

  // Reads the options from the profile / writes them back.
  void LoadSettings();  // 0x41caf0
  void SaveSettings();  // 0x41cfc0
  // Applies the options to the game (volumes, rules, ...).
  void Apply();  // 0x41c730
  // Picks the rules of `preset`; only kCustomPreset lets their menu items
  // be changed.
  void SetPreset(int preset);  // 0x41c8a0

 private:
  enum { kRules = 4, kFirstRuleItem = kStartTime };
  enum Flags {
    kFlagExit = 0x1,   // fading out to the title
    kFlagHeld = 0x2,   // a key is still held from the last change
    kFlagMusic = 0x4,  // the music was started to try the volume
  };

  // Writes the value of the formatted `item` into its text.
  void UpdateChoiceText(int item);  // 0x41d390
  // Writes the name of the trigger of the button `item` into its text.
  inline void UpdateButtonText(int item);  // 0x41d340

  // The GameRules fields of each preset, in order.
  static int presets_[][kRules];  // 0x45d9b0

  unsigned int flags_;             // 0x14, Flags
  short cursor_;                   // 0x18, the selected item
  short blink_;                    // 0x1a, alpha of the cursor, 255 down to 0
  short values_[kItems];           // 0x1c, of each menu item
  unsigned char enabled_[kItems];  // 0x40, of each menu item
  char unknown_52_[2];
  HLSTxt* items_[kItems];       // 0x54, the text of each menu item
  BackgroundGrid* background_;  // 0x9c
};

extern FnOption g_fn_option;  // 0x467cd0

#endif  // WF_FN_OPTION_H_
