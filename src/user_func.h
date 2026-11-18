// UserFunc: one stage of the program (title, game, ...) run by the
// Application's main loop (vtable 0x451c40). The default instance does
// nothing; Quit() ends the loop by finishing the current one without a
// successor.
#ifndef WF_USER_FUNC_H_
#define WF_USER_FUNC_H_

#include "object.h"

class UserFunc : public Object {
 public:
  UserFunc() {
    next_ = NULL;
    done_ = 0;
    frames_ = 0;
  }
  // Returns 0 when the stage cannot start.
  virtual int Initialize() { return 1; }
  virtual void Finalize() {}
  // Called every frame before / after the overlay tasks run.
  virtual void Move() {}
  virtual void Draw() {}

  // Ends this stage; `next` (NULL: none, ending the program) follows.
  void Finish(UserFunc* next) {
    next_ = next;
    done_ = 1;
  }

  UserFunc* next_;  // 0x08
  int done_;        // 0x0c
  int frames_;      // 0x10
};

#endif  // WF_USER_FUNC_H_
