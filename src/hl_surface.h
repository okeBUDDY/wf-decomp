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
  enum Mode {
    kPrimary = 0x1,
    kBackBuffer = 0x2,  // owned by the primary surface
    kTexture = 0x4,
    kKindMask = 0xf,
    kAttached = 0x20,  // made by someone else, see Attach()
  };
  // Pixel format codes: the bits of each channel, A last.
  enum FormatCode {
    k555 = 555,
    k5551 = 5551,
    k565 = 565,
    k4444 = 4444,
    k888 = 888,
    k8880 = 8880,
    k8888 = 8888,
  };
  enum { kFormatKinds = 6, kFormatsPerKind = 8 };

  HLSurface();  // 0x40b920

  // Reads the size and format of surface_ (`mode`: Modes).
  void Attach(unsigned short mode);  // 0x40b970
  // Fills the surface (the window area of the primary surface) with black.
  void Clear();  // 0x40b710
  // Locks the whole surface. Returns its pixels, or NULL.
  void* Lock();  // 0x407820
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
  friend class HL3D;

  // Returns the FormatCode of `format`, 0 for an unknown one.
  short GetFormatCode(const DDPIXELFORMAT* format);  // 0x40b570
  // Picks the texture format the order of kind width_ prefers, among the
  // first height_ codes; `context` is the surface.
  static HRESULT CALLBACK EnumTextureFormatProc(DDPIXELFORMAT* format,
                                                void* context);  // 0x40b680

  // The primary, opaque texture and alpha texture formats.
  static DDPIXELFORMAT formats_[3];  // 0x468f10
  static short format_codes_[3];     // 0x468f80
  // The texture formats to look for, best first, by kind.
  static short format_order_[kFormatKinds][kFormatsPerKind];  // 0x45d2b8

  IDirectDrawSurface7* surface_;  // 0x08
  void* locked_bits_;             // 0x0c, while locked
  short width_;                   // 0x10
  short height_;                  // 0x12
  short pitch_;                   // 0x14
  short format_;                  // 0x16, pixel format index
  int unknown_18_;                // 0x18
  short mode_;                    // 0x1c
  short id_;                      // 0x1e, -1: none
  bool unknown_20_;               // 0x20
  bool unknown_21_;               // 0x21
  void* buffer_;                  // 0x24, malloc()ed
};

#endif  // WF_HL_SURFACE_H_
