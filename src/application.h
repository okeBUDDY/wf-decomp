// Application: the settings and window handed to every system's
// Initialize(). Only the fields used by decompiled code are known.
#ifndef WF_APPLICATION_H_
#define WF_APPLICATION_H_

#include <windows.h>

#include "object.h"
#include "profile.h"
#include "task_list.h"

class Graphics;
class HL3D;
class HLDevice;
class HLInput;
class HLSurface;
class HLSync;
class HLWindow;
class SoundSystem;
class System;
class UserFunc;

// The startup settings, filled in before Application::Initialize().
struct AppSettings {
  // Which optional systems Initialize() starts.
  enum GraphicsFlags {
    kDirectDraw = 0x1,
    kDirect3D = 0x2,
    kInput = 0x4,
    kSound = 0x8,
  };
  enum Flags {
    kFullScreen = 0x1,
    kNoHardware3D = 0x2,  // use only the software Direct3D devices
    kRunning = 0x100,     // the main loop is running
    kSkipFrame = 0x200,   // the frame is not drawn
    kPaused = 0x400,
    kTimeStopped = 0x800,  // tasks and the stage do not move
    kStopTime = 0x1000,    // stop the time after this frame
    kStartTime = 0x2000,   // restart the time after this frame
    kResumed = 0x4000,     // set when a pause ends
    kInactive = 0x8000,
    kReset = 0x10000,  // go back to reset_func
  };
  enum Options {
    kPauseInactive = 0x2,  // pause while the window is inactive
    kLog = 0x4,            // write "<name>.log"
    kSyncKeys = 0x10,      // F1-F4 pick the HLSync mode
    kSystemKeys = 0x20,    // Esc quits, F12 resets
    kHideCursor = 0x40,    // full screen: hide the cursor
    kMessageField = 0x80,
    kClipper = 0x100,  // clip the primary surface to the window
  };

  AppSettings()
      : instance(NULL),
        window(NULL),
        graphics_flags(kDirectDraw),
        options(0x1 | 0x8 | kSyncKeys),
        flags(0),
        width(640),
        height(480),
        depth(16),
        sound_channels(32) {}

  HINSTANCE instance;    // 0x08
  HWND window;           // 0x0c
  int graphics_flags;    // 0x10, GraphicsFlags
  int options;           // 0x14, Options
  unsigned int flags;    // 0x18, Flags
  short width;           // 0x1c
  short height;          // 0x1e
  short depth;           // 0x20, bits per pixel
  short sound_channels;  // 0x22
};

struct Application : public Object, public AppSettings {
  enum { kMaxSystems = 16, kMaxDialogs = 16 };

  Application();  // 0x406f10
  // Opens the log and the profile "<name>.ini" and starts the systems
  // graphics_flags asks for. Returns 0 when one cannot start.
  int Initialize(const char* title, const char* name);  // 0x406980
  // The main loop; returns the WM_QUIT exit code.
  int Run();  // 0x406c20
  // Stops (`stop`) or restarts the time after this frame.
  void SetTimeStop(int stop);  // 0x406860
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

  int frame_count;   // 0x24, frames the time moved
  char title[0x2c];  // 0x28, "<title>  ver 0.00"
  char unknown_54_[0x24];
  TaskList tasks;                // 0x78
  TaskList overlays;             // 0x8c
  HLDevice* device;              // 0xa0
  Graphics* graphics;            // 0xa4
  HL3D* d3d;                     // 0xa8
  HLSurface* screen;             // 0xac, the back buffer
  HLInput* input;                // 0xb0
  SoundSystem* sound;            // 0xb4
  HLWindow* window_system;       // 0xb8
  HLSync* sync;                  // 0xbc
  Profile profile;               // 0xc0
  System* systems[kMaxSystems];  // 0xd0
  bool owned[kMaxSystems];       // 0x110
  int system_count;              // 0x120
  HWND dialogs[kMaxDialogs];     // 0x124, get IsDialogMessage() first
  int dialog_count;              // 0x164
  UserFunc* user_func;           // 0x168, the running stage
  UserFunc* reset_func;          // 0x16c
  // Sees every window message first; returning 0 swallows it.
  WNDPROC hook;  // 0x170
  // Handles each message of the message loop.
  void (*message_proc)(MSG* msg);  // 0x174
};

// Nonzero while the time moves (see SetTimeStop()).
extern int g_time_moves;  // 0x45d234

#endif  // WF_APPLICATION_H_
