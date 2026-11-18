// Task / TaskList: the per-frame task list (TaskList vtable 0x451fe0).
#ifndef WF_TASK_LIST_H_
#define WF_TASK_LIST_H_

#include "object.h"

class Task : public Object {
 public:
  // Returns 0 when the task could not start; TaskList::Add then deletes it.
  virtual int Initialize();
  // Called right before the task is deleted.
  virtual void Finalize();
  // Nonzero keeps the task alive in TaskList::RemoveUpTo.
  virtual int IsProtected();
  // Called every frame while the task is enabled.
  virtual void Run();

  short enabled_;            // 0x08
  unsigned short priority_;  // 0x0a
  Task* prev_;               // 0x0c
  Task* next_;               // 0x10
};

class TaskList : public Object {
 public:
  TaskList();  // 0x403af0

  void RunAll();  // 0x4039d0
  // Inserts `task` at the front (`after` == 0), at the back (`after` == 1)
  // or after the task `after`.
  void Add(Task* task, unsigned short priority, Task* after);  // 0x4039f0
  // Unlinks and deletes `task`; returns the task that followed it.
  Task* Remove(Task* task);  // 0x403aa0
  // Deletes every unprotected task with priority <= `priority`.
  void RemoveUpTo(unsigned short priority);  // 0x403b10
  // Deletes every task with priority >= `priority`.
  void RemoveFrom(unsigned short priority);  // 0x403b90
  // Deletes `task` if it is in the list.
  void Delete(Task* task);  // 0x403c00 (task_list_delete.cpp)

 private:
  Task* head_;  // 0x08
  Task* tail_;  // 0x0c
  int count_;   // 0x10 (never decremented)
};

#endif  // WF_TASK_LIST_H_
