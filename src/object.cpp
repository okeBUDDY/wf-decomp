#include "object.h"

#include <stdarg.h>
#include <stdio.h>

#include "task_list.h"

static char kObjectName[] = "UserSys";           // 0x45d188
static char kTaskListName[] = "HLFrameWorkSys";  // 0x45d190

char Object::log_path_[MAX_PATH];
char Object::message_[256];
char Object::error_message_[256];

// 0x403810
Object::Object() { name_ = kObjectName; }

// 0x403820
void Object::OpenLog(const char* path, const char* title) {
  if (log_path_[0] != '\0') {
    return;
  }
  char* file_part;
  if (GetFullPathNameA(path, MAX_PATH, log_path_, &file_part) == 0) {
    log_path_[0] = '\0';
    return;
  }
  FILE* file = fopen(log_path_, "w");
  if (file == NULL) {
    log_path_[0] = '\0';
    return;
  }
  fprintf(file, "%s\n", title);
  fclose(file);
}

// 0x403890
void Object::Log(const char* format, ...) {
  if (log_path_[0] == '\0') {
    return;
  }
  FILE* file = fopen(log_path_, "a");
  if (file == NULL) {
    return;
  }
  va_list args;
  va_start(args, format);
  vsprintf(message_, format, args);
  fprintf(file, "%s:%s\n", name_, message_);
  fclose(file);
}

// 0x4038f0
void Object::Warning(HWND window, const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsprintf(message_, format, args);
  MessageBoxA(window, message_, "WARNING", MB_ICONEXCLAMATION);
  if (log_path_[0] == '\0') {
    return;
  }
  FILE* file = fopen(log_path_, "a");
  if (file == NULL) {
    return;
  }
  fprintf(file, "%s:WARNING:%s\n", name_, message_);
  fclose(file);
}

// 0x403970
void Object::Error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsprintf(error_message_, format, args);
  if (log_path_[0] == '\0') {
    return;
  }
  FILE* file = fopen(log_path_, "a");
  if (file == NULL) {
    return;
  }
  fprintf(file, "%s:ERROR:%s\n", name_, error_message_);
  fclose(file);
}

// 0x4039d0
void TaskList::RunAll() {
  for (Task* task = head_; task != NULL; task = task->next_) {
    if (task->enabled_ != 0) {
      task->Run();
    }
  }
}

// 0x4039f0
void TaskList::Add(Task* task, unsigned short priority, Task* after) {
  if (task == NULL) {
    return;
  }
  if (!task->Initialize()) {
    task->Finalize();
    delete task;
    return;
  }
  task->priority_ = priority;
  if (count_ == 0) {
    head_ = task;
    tail_ = task;
  } else if (after == NULL) {
    head_->prev_ = task;
    task->next_ = head_;
    head_ = task;
  } else {
    if (after == TASK_APPEND) {
      after = tail_;
    }
    Task* at = head_;
    while (at != after && at != tail_) {
      at = at->next_;
    }
    task->next_ = at->next_;
    task->prev_ = at;
    at->next_ = task;
    if (at == tail_) {
      tail_ = task;
    }
  }
  count_++;
}

// 0x403aa0
Task* TaskList::Remove(Task* task) {
  Task* next = task->next_;
  if (task == head_) {
    head_ = task->next_;
  }
  if (task == tail_) {
    tail_ = task->prev_;
  }
  if (task->next_ != NULL) {
    task->next_->prev_ = task->prev_;
  }
  if (task->prev_ != NULL) {
    task->prev_->next_ = task->next_;
  }
  task->Finalize();
  delete task;
  return next;
}

// 0x403af0
TaskList::TaskList() {
  name_ = kTaskListName;
  tail_ = NULL;
  head_ = NULL;
  count_ = 0;
}

// 0x403b10
void TaskList::RemoveUpTo(unsigned short priority) {
  Task* task = head_;
  while (task != NULL) {
    if (task->priority_ <= priority && !task->IsProtected()) {
      task = Remove(task);
    } else {
      task = task->next_;
    }
  }
}

// 0x403b90
void TaskList::RemoveFrom(unsigned short priority) {
  Task* task = head_;
  while (task != NULL) {
    if (task->priority_ >= priority) {
      task = Remove(task);
    } else {
      task = task->next_;
    }
  }
}
