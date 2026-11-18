#include "thread.h"

// 0x40fca0
void Thread::Stop() {
  running_ = 0;
  if (handle_ != NULL) {
    int tries = 100;
    do {
      DWORD code;
      if (!GetExitCodeThread(handle_, &code) || code != STILL_ACTIVE) {
        break;
      }
      Sleep(20);
    } while (--tries > 0);
    if (tries == 0) {
      TerminateThread(handle_, 0);
    }
    CloseHandle(handle_);
    handle_ = NULL;
  }
  id_ = 0;
}

// 0x40fd20
DWORD WINAPI Thread::ThreadProc(void* param) {
  Thread* thread = static_cast<Thread*>(param);
  thread->exit_code_ = thread->Run();
  ExitThread(260);
}
