// fnTitle: the title screen (vtable 0x453474). Only the fields used by
// decompiled code are known so far.
#ifndef WF_FN_TITLE_H_
#define WF_FN_TITLE_H_

#include "user_func.h"

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
  char unknown_14_[0x28 - 0x14];
  HLSTxt* texts_[3];  // 0x28
  char unknown_34_[0x50 - 0x34];
  short unknown_50_;  // 0x50
};

// The background of the title screen, kept in Application::tasks.
extern Task* g_title_background;  // 0x469278

#endif  // WF_FN_TITLE_H_
