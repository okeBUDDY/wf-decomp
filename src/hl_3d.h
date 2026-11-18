// HL3D: the Direct3D 7 device drawn into the back buffer (vtable 0x451568).
// Only the fields used by decompiled code are known so far.
#ifndef WF_HL_3D_H_
#define WF_HL_3D_H_

#include <windows.h>

#include <d3d.h>

#include "system.h"

struct Application;

class HL3D : public System {
 public:
  enum Flags { kInScene = 0x10000 };

  HL3D();                                    // 0x402bb0
  virtual int Initialize(Application* app);  // 0x4030c0
  virtual void Finalize();                   // 0x402e60
  virtual void Pause() {}
  virtual void Resume();  // 0x402990

  // Starts the frame's scene unless one is open.
  void BeginScene() {
    if (!(flags_ & kInScene) && device_->BeginScene() == D3D_OK) {
      flags_ |= kInScene;
    }
  }
  // Clears the target (and the z-buffer when there is one).
  void Clear() {
    device_->Clear(0, NULL,
                   (zbuffer_ != NULL ? D3DCLEAR_ZBUFFER : 0) | D3DCLEAR_TARGET,
                   0, 1.0f, 0);
  }
  void EndScene() {
    if (flags_ & kInScene) {
      device_->EndScene();
      flags_ &= ~kInScene;
    }
  }

 private:
  IDirect3D7* d3d_;               // 0x08
  IDirect3DDevice7* device_;      // 0x0c
  int unknown_10_;                // 0x10
  IDirectDrawSurface7* zbuffer_;  // 0x14
  short depth_;                   // 0x18, z-buffer bits
  Application* app_;              // 0x1c
  int unknown_20_;                // 0x20
  unsigned int flags_;            // 0x24, Flags
  char unknown_28_[0x44];
  int unknown_6c_;  // 0x6c
  int unknown_70_;  // 0x70
  int unknown_74_;  // 0x74
};

#endif  // WF_HL_3D_H_
