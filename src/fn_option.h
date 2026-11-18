// fnOption: the option screen (vtable 0x453354, g_fn_option); it also
// reads and writes the [Option] section of the profile. Only the fields
// used by decompiled code are known so far.
#ifndef WF_FN_OPTION_H_
#define WF_FN_OPTION_H_

#include "game_rules.h"
#include "user_func.h"

class BackgroundGrid;
class HLSTxt;

class FnOption : public UserFunc {
 public:
  enum { kItems = 18, kCustomPreset = 4 };

  FnOption();                // 0x41c6e0
  virtual int Initialize();  // 0x41dc60
  virtual void Finalize();   // 0x41ded0
  virtual void Move();       // 0x41d4d0
  virtual void Draw();       // 0x41d880

  // Reads the options from the profile.
  void LoadSettings();  // 0x41caf0
  // Picks the rules of `preset`; only kCustomPreset lets their menu items
  // be changed.
  void SetPreset(int preset);  // 0x41c8a0

 private:
  enum { kRules = 4, kFirstRuleItem = 8 };

  // The GameRules fields of each preset, in order.
  static int presets_[][kRules];  // 0x45d9b0

  char unknown_14_[0x2c - 0x14];
  short rules_[kRules];  // 0x2c, the fields of a GameRules
  char unknown_34_[0x40 - 0x34];
  bool enabled_[kItems];  // 0x40, of each menu item
  char unknown_52_[2];
  HLSTxt* items_[kItems];       // 0x54, the text of each menu item
  BackgroundGrid* background_;  // 0x9c
};

extern FnOption g_fn_option;  // 0x467cd0

#endif  // WF_FN_OPTION_H_
