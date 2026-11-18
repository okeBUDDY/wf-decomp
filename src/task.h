// TaskBase: a Node that unlinks itself from the tree when destroyed
// (vtable 0x451544).
#ifndef WF_TASK_H_
#define WF_TASK_H_

#include "node.h"

class TaskBase : public Node {
 public:
  TaskBase();           // 0x402780
  virtual ~TaskBase();  // 0x402880, scalar deleting dtor 0x402860
};

#endif  // WF_TASK_H_
