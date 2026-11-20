// fnTitle: the title screen (vtable 0x453474).
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
    kFlagTitleShown = 0x2,  // the intro is over (or was skipped)
  };
  // step_: the steps of the title screen.
  enum Step {
    kStepLogo = 100,      // 100-104: the logo fades in and out
    kStepIntro = 200,     // 200-206: the title sweeps in
    kStepTitle = 300,     // the title is complete
    kStepMenu = 301,      // 301-302: the menu
    kStepDemo = 400,      // 400-401: fade out and play a replay
    kStepSelected = 500,  // 500-501: fade out and run the chosen item
  };
  // menu_: the menu items (kMenuItems).
  enum MenuItem {
    kMenuStart = 0,
    kMenuReplay = 1,
    kMenuOption = 2,
    kMenuExit = 3,
    kMenuItemCount = 4,
  };

  Actor* logo_[2];       // 0x14, the two halves of the logo (etc2.pat)
  Actor* sweeps_[2];     // 0x1c, the bars that sweep in (etc0.pat)
  Actor* subtitle_;      // 0x24, draws texts_[0]
  HLSTxt* texts_[3];     // 0x28: the subtitle, the menu item, the scores
  float sweep_offset_;   // 0x34, of the bars that sweep in
  int unused_38_;        // 0x38, never accessed
  float hex_scale_;      // 0x3c, of the spinning hexagon
  unsigned int flags_;   // 0x40, Flags
  short cursor_alpha_;   // 0x44
  short logo_alpha_;     // 0x46
  short title_shown_;    // 0x48, characters of kTitle typed out
  short title_length_;   // 0x4a, of kTitle
  short scores_shown_;   // 0x4c, characters of the score table typed out
  short scores_length_;  // 0x4e
  short menu_;           // 0x50, MenuItem
  short menu_x_;         // 0x52, of the menu item
  short step_;           // 0x54, Step
  short step_time_;      // 0x56, frames in the current step
};

extern FnTitle g_fn_title;  // 0x467c40

// The background of the title screen, kept in Application::tasks.
extern Task* g_title_background;  // 0x469278

#endif  // WF_FN_TITLE_H_
