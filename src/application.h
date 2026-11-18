// Application: the settings and window handed to every system's
// Initialize(). Only the fields used by decompiled code are known.
#ifndef WF_APPLICATION_H_
#define WF_APPLICATION_H_

#include <windows.h>

#include "profile.h"

class Graphics;

struct Application {
  enum ScreenFlags { kFullScreen = 0x1 };
  enum Flags {
    kPageFlip = 0x1,     // full screen: present by flipping
    kSkipFrame = 0x200,  // the frame is not drawn
  };

  char unknown_00_[0xc];
  HWND window;                 // 0x0c
  unsigned char screen_flags;  // 0x10, ScreenFlags
  char unknown_11_[0x7];
  unsigned int flags;  // 0x18, Flags
  char unknown_1c_[0x6];
  short sound_channels;  // 0x22
  char unknown_24_[0x80];
  Graphics* graphics;  // 0xa4
  char unknown_a8_[0x18];
  Profile profile;  // 0xc0
};

#endif  // WF_APPLICATION_H_
