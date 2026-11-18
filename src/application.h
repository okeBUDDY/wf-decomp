// Application: the settings and window handed to every system's
// Initialize(). Only the fields used by decompiled code are known.
#ifndef WF_APPLICATION_H_
#define WF_APPLICATION_H_

#include <windows.h>

#include "object.h"
#include "profile.h"
#include "task_list.h"

class Graphics;
class HLSync;
class System;
class UserFunc;

struct Application : public Object {
  enum GraphicsFlags { kDirectDraw = 0x1 };
  enum Flags {
    kFullScreen = 0x1,
    kRunning = 0x100,    // the main loop is running
    kSkipFrame = 0x200,  // the frame is not drawn
    kPaused = 0x400,
    kResumed = 0x4000,  // set when a pause ends
    kInactive = 0x8000,
    kReset = 0x10000,  // go back to reset_func
  };

  enum { kMaxSystems = 16 };

  // Pauses every system (`pause`) or ends the pause.
  void Pause(int pause);  // 0x4067f0
  // The default message_proc.
  static void DispatchMsg(MSG* msg);  // 0x4067d0
  // Initializes `system` and keeps it until Shutdown() (which deletes it
  // when `owns`). Returns 0 when it cannot be added or initialized.
  int AddSystem(System* system, bool owns);  // 0x406890
  // Shows the last error, then finalizes the systems, last first.
  void Shutdown();  // 0x406b80
  // Ends the main loop (or the process when it is not running).
  void Quit();  // 0x4071b0
  // Asks the main loop to restart at reset_func (when there is one).
  void RequestReset();  // 0x40cda0

  enum Options {
    kPauseInactive = 0x2,  // pause while the window is inactive
    kSyncKeys = 0x10,      // F1-F4 pick the HLSync mode
    kSystemKeys = 0x20,    // Esc quits, F12 resets
    kMessageField = 0x80,
  };

  HINSTANCE instance;            // 0x08
  HWND window;                   // 0x0c
  unsigned char graphics_flags;  // 0x10, GraphicsFlags
  char unknown_11_[0x3];
  signed char options;  // 0x14, Options
  char unknown_15_[0x3];
  unsigned int flags;  // 0x18, Flags
  short width;         // 0x1c
  short height;        // 0x1e
  char unknown_20_[0x2];
  short sound_channels;  // 0x22
  char unknown_24_[0x54];
  TaskList tasks;     // 0x78
  TaskList overlays;  // 0x8c
  char unknown_a0_[0x4];
  Graphics* graphics;  // 0xa4
  char unknown_a8_[0x14];
  HLSync* sync;                  // 0xbc
  Profile profile;               // 0xc0
  System* systems[kMaxSystems];  // 0xd0
  bool owned[kMaxSystems];       // 0x110
  int system_count;              // 0x120
  char unknown_124_[0x44];
  UserFunc* user_func;   // 0x168, the running stage
  UserFunc* reset_func;  // 0x16c
  // Sees every window message first; returning 0 swallows it.
  WNDPROC hook;  // 0x170
  // Handles each message of the message loop.
  void (*message_proc)(MSG* msg);  // 0x174
};

#endif  // WF_APPLICATION_H_
