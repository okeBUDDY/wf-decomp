// HLSurface: a DirectDraw surface with its size (vtable 0x451fe0). Only
// the fields used by decompiled code are known so far.
#ifndef WF_HL_SURFACE_H_
#define WF_HL_SURFACE_H_

#include <windows.h>

#include <ddraw.h>

#include "object.h"

class Graphics;
struct IDirect3DDevice7;

class HLSurface : public Object {
 public:
  enum Mode {
    kPrimary = 0x1,
    kBackBuffer = 0x2,  // owned by the primary surface
    kOffscreen = 0x3,
    kTexture = 0x4,
    kKindMask = 0xf,
    kSystemMemory = 0x10,  // in system memory (no hardware 3D)
    kAttached = 0x20,      // made by someone else, see Attach()
    kRenderTarget = 0x20,  // Create(): an offscreen 3D render target
    kAlpha = 0x40,         // with an alpha channel (32-bit images)
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
  // Draws `text` at (`x`, `y`) with GDI in `color`, in the font `face`
  // `height` pixels high (the window's fixed font when NULL); stores the
  // rectangle drawn in `rect` when not NULL.
  void Print(int x, int y, const char* text, COLORREF color, const char* face,
             int height, int bold, int italic, RECT* rect);  // 0x40b780
  // Locks the whole surface. Returns its pixels, or NULL.
  void* Lock();  // 0x407820
  // Unlocks the surface if it is locked.
  void Unlock() {
    if (locked_bits_ != NULL) {
      surface_->Unlock(NULL);
      locked_bits_ = NULL;
    }
  }
  // Releases the surface and the buffer.
  void Release();  // 0x40ba80
  // Creates a `width` x `height` surface. Returns 0 on error.
  int Create(int width, int height, unsigned short mode);  // 0x40bda0
  // Picks the texture formats `device` supports; 0 when one is missing.
  int InitTextureFormats(IDirect3DDevice7* device);  // 0x40bb10
  // Loads the image `file` (from data.bin) into the surface, making it
  // when needed (`mode`: Modes). Returns 0 on error.
  int Load(const char* file, unsigned short mode);  // 0x40bff0
  // Restores the surface after a mode change, reloading its pixels.
  void Restore() {
    if (surface_ != NULL && surface_->IsLost() == DDERR_SURFACELOST) {
      surface_->Restore();
      if (file_name_ != NULL) {
        Load(file_name_, mode_);
      }
    }
  }

  // The display surfaces are made with.
  static Graphics* graphics_;  // 0x468f70

  IDirectDrawSurface7* surface() const { return surface_; }

 private:
  friend class Graphics;
  friend class HL3D;
  friend class HLSTxtSys;

  // Returns the FormatCode of `format`, 0 for an unknown one.
  short GetFormatCode(const DDPIXELFORMAT* format);  // 0x40b570
  // Picks the texture format the order of kind width_ prefers, among the
  // first height_ codes; `context` is the surface.
  static HRESULT CALLBACK EnumTextureFormatProc(DDPIXELFORMAT* format,
                                                void* context);  // 0x40b680

  // texture_caps_.
  enum TextureCaps {
    kSquareOnly = 0x1,  // textures must be square
    kPow2 = 0x2,        // texture sizes must be powers of 2
    kNoBlendOne = 0x4,  // the device cannot blend with D3DBLEND_ONE
  };
  enum { kMaxSurfaces = 0x80 };

  // What the texture formats found allow (set by InitTextureFormats()).
  static unsigned int texture_caps_;  // 0x468f74, TextureCaps
  // The largest texture size; bigger images are scaled down.
  static short max_texture_width_;   // 0x468f78
  static short max_texture_height_;  // 0x468f7c
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
  unsigned char scale_x_;         // 0x20, texels per pixel
  unsigned char scale_y_;         // 0x21
  char* file_name_;               // 0x24, of the image loaded, malloc()ed
};

#endif  // WF_HL_SURFACE_H_
