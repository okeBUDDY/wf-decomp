#include "hl_surface.h"

#include <stdlib.h>

#include "application.h"
#include "graphics.h"

Graphics* HLSurface::graphics_;        // 0x468f70
DDPIXELFORMAT HLSurface::formats_[3];  // 0x468f10
short HLSurface::format_codes_[3];     // 0x468f80
short HLSurface::format_order_[kFormatKinds][kFormatsPerKind] = {
    {k565, k5551, k555, k4444, k888, k8880, k8888, 0},
    {k565, k555, k5551, k4444, k888, k8880, k8888, 0},
    {k888, k8880, k8888, k565, k555, k5551, k4444, 0},
    {k4444, k8888, k5551, k555, k565, k888, k8880, 0},
    {k4444, k8888, k5551, k565, k555, k888, k8880, 0},
    {k8888, k4444, k5551, k888, k8880, k565, k555, 0},
};  // 0x45d2b8

// 0x40b570
short HLSurface::GetFormatCode(const DDPIXELFORMAT* format) {
  switch (format->dwRGBBitCount) {
    case 32:
      if (format->dwBBitMask == 0xff && format->dwGBitMask == 0xff00 &&
          format->dwRBitMask == 0xff0000) {
        return format->dwRGBAlphaBitMask == 0xff000000 ? k8888 : k8880;
      }
      break;
    case 24:
      if (format->dwBBitMask == 0xff && format->dwGBitMask == 0xff00 &&
          format->dwRBitMask == 0xff0000) {
        return k888;
      }
      break;
    case 16:
      if (format->dwBBitMask == 0x1f && format->dwGBitMask == 0x3e0 &&
          format->dwRBitMask == 0x7c00) {
        return format->dwRGBAlphaBitMask == 0x8000 ? k5551 : k555;
      }
      if (format->dwBBitMask == 0x1f && format->dwGBitMask == 0x7e0 &&
          format->dwRBitMask == 0xf800) {
        return k565;
      }
      if (format->dwBBitMask == 0xf && format->dwGBitMask == 0xf0 &&
          format->dwRBitMask == 0xf00 && format->dwRGBAlphaBitMask == 0xf000) {
        return k4444;
      }
      break;
  }
  return 0;
}

// 0x40b680
HRESULT CALLBACK HLSurface::EnumTextureFormatProc(DDPIXELFORMAT* format,
                                                  void* context) {
  HLSurface* surface = static_cast<HLSurface*>(context);
  short kind = surface->width_;
  int slot = (kind >= 3) + 1;
  short code = surface->GetFormatCode(format);
  for (int i = 0; i < kFormatsPerKind; ++i) {
    if (format_order_[kind][i] == code && i < surface->height_ &&
        i != kFormatsPerKind - 1) {
      formats_[slot] = *format;
      format_codes_[slot] = code;
      surface->height_ = i;
      return DDENUMRET_OK;
    }
  }
  return DDENUMRET_OK;
}

// 0x40b710
void HLSurface::Clear() {
  if (surface_ == NULL) {
    return;
  }
  DDBLTFX fx;
  fx.dwSize = sizeof(fx);
  fx.dwFillColor = 0;
  if (!(graphics_->app_->flags & Application::kFullScreen) &&
      (mode_ & kKindMask) == kPrimary) {
    surface_->Blt(&graphics_->window_rect_, NULL, NULL,
                  DDBLT_COLORFILL | DDBLT_WAIT, &fx);
  } else {
    surface_->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
  }
}

// 0x40b920
// TODO: nonmatching: the original keeps eax = 0 and loads the `true` into
// cl before the first store; the stores are the same.
HLSurface::HLSurface() {
  surface_ = NULL;
  locked_bits_ = NULL;
  width_ = height_ = pitch_ = format_ = mode_ = 0;
  buffer_ = NULL;
  unknown_18_ = 0;
  name_ = "  HLSurface";
  unknown_20_ = unknown_21_ = true;
  id_ = -1;
}

// 0x40b970
void HLSurface::Attach(unsigned short mode) {
  if (surface_ == NULL) {
    return;
  }
  if (mode != 0) {
    mode_ = mode | kAttached;
    format_ = 0;
    id_ = -1;
  }
  DDSURFACEDESC2 desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwSize = sizeof(desc);
  surface_->GetSurfaceDesc(&desc);
  width_ = static_cast<short>(desc.dwWidth);
  height_ = static_cast<short>(desc.dwHeight);
  pitch_ = static_cast<short>(desc.lPitch);
  locked_bits_ = NULL;
  unknown_18_ = 0;
  if ((mode_ & kKindMask) == kPrimary) {
    formats_[0] = desc.ddpfPixelFormat;
    format_codes_[0] = GetFormatCode(&desc.ddpfPixelFormat);
  }
  Clear();
  // "[%d]%s created: mode=%04X %dx%d(%d:%d) %dbit(%d)"
  Log("[%d]%s\220\266\220\254: mode=%04X %dx%d(%d:%d) %dbit(%d)", id_,
      (mode_ & kKindMask) == kTexture ? "\303\270\275\301\254"   // texture
                                      : "\273\260\314\252\275",  // surface
      mode_, width_, height_, unknown_20_, unknown_21_,
      desc.ddpfPixelFormat.dwRGBBitCount, format_codes_[format_]);
}

// 0x40ba80
void HLSurface::Release() {
  if (surface_ == NULL) {
    return;
  }
  if (locked_bits_ != NULL) {
    surface_->Unlock(NULL);
    locked_bits_ = NULL;
  }
  if (buffer_ != NULL) {
    free(buffer_);
  }
  buffer_ = NULL;
  if ((mode_ & kKindMask) != kBackBuffer) {
    surface_->Release();
  }
  surface_ = NULL;
  // "[%d] surface deleted"
  Log("[%d]\273\260\314\252\275\215\355\217\234", id_);
  if (id_ != -1) {
    graphics_->surfaces_[id_] = NULL;
  }
  unknown_18_ = 0;
  id_ = -1;
  mode_ = 0;
}
