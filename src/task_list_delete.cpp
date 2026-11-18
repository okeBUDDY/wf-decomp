// TaskList::Delete lives in a different object than the rest of TaskList:
// it tail-calls Remove instead of inlining it, while RemoveUpTo/RemoveFrom in
// object.cpp inline Remove (the game is built with /Ob2).
#include "task_list.h"

// 0x403c00
void TaskList::Delete(Task* task) {
  if (task == NULL) {
    return;
  }
  for (Task* at = head_; at != NULL; at = at->next_) {
    if (at == task) {
      Remove(at);
      return;
    }
  }
}
