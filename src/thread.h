// Thread: a Win32 thread running Run() (vtable 0x451dd0).
#ifndef WF_THREAD_H_
#define WF_THREAD_H_

#include <windows.h>

class Thread {
 public:
  Thread() : id_(0), handle_(NULL), running_(0), exit_code_(0) {}
  // 0x408ec0, scalar deleting dtor 0x408e80
  virtual ~Thread() {}
  // The thread body; its result becomes the exit code.
  virtual int Run() = 0;

  // Asks Run() to return and waits up to two seconds before killing the
  // thread.
  void Stop();  // 0x40fca0
  HANDLE handle() const { return handle_; }

 protected:
  friend class SoundSystem;

  static DWORD WINAPI ThreadProc(void* param);  // 0x40fd20

  DWORD id_;        // 0x04
  HANDLE handle_;   // 0x08
  int running_;     // 0x0c, cleared to stop Run()
  int exit_code_;   // 0x10
};

#endif  // WF_THREAD_H_
