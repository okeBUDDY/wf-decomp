// TaskBase: a node in the game's task tree (vtable 0x451544).
#ifndef WF_TASK_H_
#define WF_TASK_H_

#include "object.h"

class TaskBase : public Object {
 public:
  virtual ~TaskBase();  // 0x402880, scalar deleting dtor 0x402860

  // 0x4027a0: deletes all children, then detaches this node from its parent.
  void Unlink();

 private:
  // Fields from 0x04 to 0xcf are not recovered yet. The tree links below
  // are read off Unlink() and not verified elsewhere.
  char unknown04_[0xd0 - 0x04];
  TaskBase* parent_;       // 0xd0
  TaskBase* first_child_;  // 0xd4
  TaskBase* next_;         // 0xd8
};

#endif  // WF_TASK_H_
