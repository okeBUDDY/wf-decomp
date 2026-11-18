// Application: the settings and window handed to every system's
// Initialize(). Only the fields used by decompiled code are known.
#ifndef WF_APPLICATION_H_
#define WF_APPLICATION_H_

#include <windows.h>

#include "profile.h"

class Graphics;
class HLSync;

struct Application {
  enum GraphicsFlags { kDirectDraw = 0x1 };
  enum Flags {
    kFullScreen = 0x1,
    kDrawing = 0x100,
    kSkipFrame = 0x200,  // the frame is not drawn
    kPaused = 0x400,
    kInactive = 0x8000,
    kCapture = 0x10000,
  };

  void Pause(int pause);  // 0x4067f0
  // Ends the main loop (or the process when it is not running).
  void Quit();  // 0x4071b0
  // Asks for a capture of the next frame when capturing is set up.
  void RequestCapture();  // 0x40cda0

  enum Options {
    kPauseInactive = 0x2,  // pause while the window is inactive
    kSyncKeys = 0x10,      // F1-F4 pick the HLSync mode
    kSystemKeys = 0x20,    // Esc quits, F12 requests a capture
    kMessageField = 0x80,
  };

  char unknown_00_[0x8];
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
  char unknown_24_[0x80];
  Graphics* graphics;  // 0xa4
  char unknown_a8_[0x14];
  HLSync* sync;     // 0xbc
  Profile profile;  // 0xc0
  char unknown_d0_[0x9c];
  void* capture_;  // 0x16c
  // Sees every window message first; returning 0 swallows it.
  WNDPROC hook;  // 0x170
};

#endif  // WF_APPLICATION_H_
