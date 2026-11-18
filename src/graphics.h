// Graphics: the DirectDraw display, "HLDraw" in the log (vtable 0x4518fc).
// Only the fields used by decompiled code are known so far.
#ifndef WF_GRAPHICS_H_
#define WF_GRAPHICS_H_

#include <windows.h>

#include <ddraw.h>

#include "hl_surface.h"
#include "system.h"

struct Application;

class Graphics : public System {
 public:
  Graphics();                                // 0x4053e0
  virtual int Initialize(Application* app);  // 0x405480
  virtual void Finalize();                   // 0x405810
  virtual void Pause();                      // 0x4058e0
  virtual void Resume();                     // 0x405910

  // Copies the back buffer to the window area of the primary surface.
  void Blt() {
    primary_.surface_->Blt(&window_rect_, back_.surface_, NULL, DDBLT_WAIT,
                           NULL);
  }  // 0x40c730
  // Copies the back buffer to the top left of the primary surface.
  void BltFast() {
    primary_.surface_->BltFast(0, 0, back_.surface_, NULL, DDBLTFAST_WAIT);
  }
  void Flip() { primary_.surface_->Flip(NULL, DDFLIP_WAIT); }
  // Follows the window after it moved or was resized.
  void UpdateWindowRect(HWND window);  // 0x405300

  // The back buffer everything is drawn to.
  HLSurface* back() { return &back_; }

 private:
  IDirectDraw7* ddraw_;  // 0x08
  int unknown_0c_;       // 0x0c
  int unknown_10_;       // 0x10
  int unknown_14_;       // 0x14
  int unknown_18_;       // 0x18
  RECT window_rect_;     // 0x1c
  HLSurface primary_;    // 0x2c
  HLSurface back_;       // 0x54
  int unknown_7c_;       // 0x7c
};

#endif  // WF_GRAPHICS_H_
