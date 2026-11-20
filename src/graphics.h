// Graphics: the DirectDraw display, "HLDraw" in the log (vtable 0x4518fc).
#ifndef WF_GRAPHICS_H_
#define WF_GRAPHICS_H_

#include <windows.h>

#include <ddraw.h>

#include "hl_surface.h"
#include "system.h"

struct Application;

class HLDevSet;

class Graphics : public System {
 public:
  enum { kMaxSurfaces = 128 };
  enum Flags {
    kDisplayModeSet = 0x1,  // full screen: restore the mode at the end
    kCursorHidden = 0x2,
  };

  Graphics();                                // 0x4053e0
  virtual int Initialize(Application* app);  // 0x405480
  virtual void Finalize();                   // 0x405810
  // Frees the cursor.
  virtual void Pause();  // 0x4058e0
  // Clips the cursor again and restores the lost surfaces.
  virtual void Resume();  // 0x405910

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
  // Full screen: keeps the cursor in the window (`clip`) or frees it.
  void ClipCursor(int clip);  // 0x405350
  // Adds the DirectDraw devices with 3D textures to `set`. Returns 0 when
  // there is none.
  int EnumDevices(HLDevSet* set);  // 0x405a30
  // Uses the display `device` (NULL: the primary one).
  void set_device(GUID* device) { device_ = device; }

  // The back buffer everything is drawn to.
  HLSurface* back() { return &back_; }

 private:
  friend class HL3D;
  friend class HLSurface;

  static BOOL WINAPI EnumDeviceProc(GUID* guid, LPSTR description, LPSTR name,
                                    LPVOID context,
                                    HMONITOR monitor);  // 0x405270

  IDirectDraw7* ddraw_;          // 0x08
  GUID* device_;                 // 0x0c, NULL: the primary display
  Application* app_;             // 0x10
  unsigned int flags_;           // 0x14, Flags
  IDirectDrawClipper* clipper_;  // 0x18
  RECT window_rect_;             // 0x1c, in screen coordinates
  HLSurface primary_;            // 0x2c
  HLSurface back_;               // 0x54
  // 0x7c, malloc()ed: the surfaces to restore after a mode change.
  HLSurface** surfaces_;
};

#endif  // WF_GRAPHICS_H_
