// HLSurface: a DirectDraw surface with its size (vtable 0x451fe0). Only
// the fields used by decompiled code are known so far.
#ifndef WF_HL_SURFACE_H_
#define WF_HL_SURFACE_H_

#include <windows.h>

#include <ddraw.h>

#include "object.h"

class Graphics;

class HLSurface : public Object {
 public:
  HLSurface();  // 0x40b920

  // Reads the size and format of surface_ (`mode`: Modes).
  void Attach(unsigned short mode);  // 0x40b970
  // Releases the surface and the buffer.
  void Release();  // 0x40ba80
  // Creates a `width` x `height` surface. Returns 0 on error.
  int Create(int width, int height, unsigned short mode);  // 0x40bda0
  // Copies `pixels` into the surface.
  void Load(void* pixels, unsigned short mode);  // 0x40bff0
  // Restores the surface after a mode change, reloading its pixels.
  void Restore() {
    if (surface_ != NULL && surface_->IsLost() == DDERR_SURFACELOST) {
      surface_->Restore();
      if (buffer_ != NULL) {
        Load(buffer_, mode_);
      }
    }
  }

  // The display surfaces are made with.
  static Graphics* graphics_;  // 0x468f70

  IDirectDrawSurface7* surface() const { return surface_; }

 private:
  friend class Graphics;

  IDirectDrawSurface7* surface_;  // 0x08
  void* locked_bits_;             // 0x0c, while locked
  short width_;                   // 0x10
  short height_;                  // 0x12
  short pitch_;                   // 0x14
  short format_;                  // 0x16, pixel format index
  int unknown_18_;                // 0x18
  unsigned short mode_;           // 0x1c
  short id_;                      // 0x1e, -1: none
  bool unknown_20_;               // 0x20
  bool unknown_21_;               // 0x21
  void* buffer_;                  // 0x24, malloc()ed
};

#endif  // WF_HL_SURFACE_H_
