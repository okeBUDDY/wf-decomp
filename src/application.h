// Application: the settings and window handed to every system's
// Initialize(). Only the fields used by decompiled code are known.
#ifndef WF_APPLICATION_H_
#define WF_APPLICATION_H_

#include <windows.h>

struct Application {
  char unknown_00_[0xc];
  HWND window;  // 0x0c
  char unknown_10_[0x12];
  short sound_channels;  // 0x22
};

#endif  // WF_APPLICATION_H_
