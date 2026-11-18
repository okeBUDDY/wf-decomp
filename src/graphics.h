// Graphics: the DirectDraw display. Only the fields used by decompiled code
// are known so far.
#ifndef WF_GRAPHICS_H_
#define WF_GRAPHICS_H_

#include <windows.h>

#include <ddraw.h>

class Graphics {
 public:
  // Copies the back buffer to the window area of the primary surface.
  void Blt() {
    primary_->Blt(&window_rect_, back_, NULL, DDBLT_WAIT, NULL);
  }  // 0x40c730
  // Copies the back buffer to the top left of the primary surface.
  void BltFast() { primary_->BltFast(0, 0, back_, NULL, DDBLTFAST_WAIT); }
  void Flip() { primary_->Flip(NULL, DDFLIP_WAIT); }
  // Follows the window after it moved or was resized.
  void UpdateWindowRect(HWND window);  // 0x405300

 private:
  char unknown_00_[0x1c];
  RECT window_rect_;  // 0x1c
  char unknown_2c_[0x8];
  IDirectDrawSurface7* primary_;  // 0x34
  char unknown_38_[0x24];
  IDirectDrawSurface7* back_;  // 0x5c
};

#endif  // WF_GRAPHICS_H_
