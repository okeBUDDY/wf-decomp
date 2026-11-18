// fnTitle: the title screen (vtable 0x453474). Only the fields used by
// decompiled code are known so far.
#ifndef WF_FN_TITLE_H_
#define WF_FN_TITLE_H_

#include "user_func.h"

class Actor;
class HLSTxt;
class Task;

class FnTitle : public UserFunc {
 public:
  FnTitle();                 // 0x41df30
  virtual int Initialize();  // 0x41e050
  virtual void Finalize();   // 0x41df70
  virtual void Move();       // 0x41e300
  virtual void Draw();       // 0x41eae0

 private:
  enum Flags {
    kFlagMenuCursor = 0x1,  // the arrows around the menu item are shown
  };

  Actor* logo_[2];    // 0x14, the two halves of the logo (etc2.pat)
  Actor* sweeps_[2];  // 0x1c, the bars that sweep in (etc0.pat)
  Actor* subtitle_;   // 0x24, draws texts_[0]
  HLSTxt* texts_[3];  // 0x28: the subtitle, the menu item, the scores
  char unknown_34_[0x3c - 0x34];
  float hex_scale_;      // 0x3c, of the spinning hexagon
  unsigned int flags_;   // 0x40, Flags
  short cursor_alpha_;   // 0x44
  short unknown_46_;     // 0x46
  short unknown_48_;     // 0x48
  short title_length_;   // 0x4a, of kTitle
  short scores_shown_;   // 0x4c, characters of the score table typed out
  short scores_length_;  // 0x4e
  short unknown_50_;     // 0x50
  short menu_x_;         // 0x52, of the menu item
  short time_;           // 0x54
  short unknown_56_;     // 0x56
};

// The background of the title screen, kept in Application::tasks.
extern Task* g_title_background;  // 0x469278

#endif  // WF_FN_TITLE_H_
