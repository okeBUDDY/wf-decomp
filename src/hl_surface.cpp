#include "hl_surface.h"

#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "graphics.h"
#include "hl_image.h"

Graphics* HLSurface::graphics_;         // 0x468f70
DDPIXELFORMAT HLSurface::formats_[3];   // 0x468f10
short HLSurface::format_codes_[3];      // 0x468f80
unsigned int HLSurface::texture_caps_;  // 0x468f74
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
  file_name_ = NULL;
  unknown_18_ = 0;
  name_ = "  HLSurface";
  scale_x_ = scale_y_ = 1;
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
  Log("[%d]%s生成: mode=%04X %dx%d(%d:%d) %dbit(%d)", id_,
      (mode_ & kKindMask) == kTexture ? "ﾃｸｽﾁｬ"   // texture
                                      : "ｻｰﾌｪｽ",  // surface
      mode_, width_, height_, scale_x_, scale_y_,
      desc.ddpfPixelFormat.dwRGBBitCount, format_codes_[format_]);
}

// 0x40ba80
void HLSurface::Release() {
  if (surface_ == NULL) {
    return;
  }
  Unlock();
  if (file_name_ != NULL) {
    free(file_name_);
  }
  file_name_ = NULL;
  if ((mode_ & kKindMask) != kBackBuffer) {
    surface_->Release();
  }
  surface_ = NULL;
  // "[%d] surface deleted"
  Log("[%d]ｻｰﾌｪｽ削除", id_);
  if (id_ != -1) {
    graphics_->surfaces_[id_] = NULL;
  }
  unknown_18_ = 0;
  id_ = -1;
  mode_ = 0;
}

// 0x40bff0
// TODO: nonmatching: the pixel conversions match, but the original keeps
// `file` in ebx and `this` in edi (spilled), and keeps the loop counters on
// the stack differently (0x33 bytes longer).
int HLSurface::Load(const char* file, unsigned short mode) {
  int result = 0;
  HLImage image;
  if (image.Load(file, HLImage::kAuto)) {
    if (file_name_ != NULL && strcmp(file, file_name_) != 0) {
      Release();
    }
    switch (image.bits()) {
      case 24:
        mode &= ~kAlpha;
        break;
      case 32:
        mode |= kAlpha;
        break;
    }
    if (surface_ != NULL || Create(image.width(), image.height(), mode)) {
      unsigned char* dest = static_cast<unsigned char*>(Lock());
      if (dest != NULL) {
        int bytes = image.bits() / 8;
        // Every scale_x_-th pixel of every scale_y_-th row.
        int row_skip = (scale_y_ - 1) * bytes * image.width();
        int step = scale_x_ * bytes;
        int width = image.width() / scale_x_;
        int height = image.height() / scale_y_;
        const unsigned char* src = image.pixels();
        switch (format_codes_[format_]) {
          case k888: {
            int pad = pitch_ - width * 3;
            for (int y = 0; y < height; ++y) {
              for (int x = 0; x < width; ++x) {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                src += step;
                dest += 3;
              }
              src += row_skip;
              dest += pad;
            }
            break;
          }
          case k565: {
            unsigned short* out = reinterpret_cast<unsigned short*>(dest);
            int pad = pitch_ / 2 - width;
            for (int y = 0; y < height; ++y) {
              for (int x = 0; x < width; ++x) {
                *out++ = (((src[2] & 0xf8) << 5 | (src[1] & 0xfc)) << 3) |
                         src[0] >> 3;
                src += step;
              }
              src += row_skip;
              out += pad;
            }
            break;
          }
          case k555: {
            unsigned short* out = reinterpret_cast<unsigned short*>(dest);
            int pad = pitch_ / 2 - width;
            for (int y = 0; y < height; ++y) {
              for (int x = 0; x < width; ++x) {
                *out++ = (((src[2] & 0xf8) << 5 | (src[1] & 0xf8)) << 2) |
                         src[0] >> 3;
                src += step;
              }
              src += row_skip;
              out += pad;
            }
            break;
          }
          case k4444: {
            unsigned short* out = reinterpret_cast<unsigned short*>(dest);
            int pad = pitch_ / 2 - width;
            if (image.bits() == 24) {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  *out++ = ((0xff00 | (src[2] & 0xf0)) << 4) |
                           ((src[1] & 0xf0) | src[0] >> 4);
                  src += step;
                }
                src += row_skip;
                out += pad;
              }
            } else {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  *out++ = (((src[3] & 0xf0) << 4 | (src[2] & 0xf0)) << 4) |
                           ((src[1] & 0xf0) | src[0] >> 4);
                  src += step;
                }
                src += row_skip;
                out += pad;
              }
            }
            break;
          }
          case k5551: {
            unsigned short* out = reinterpret_cast<unsigned short*>(dest);
            int pad = pitch_ / 2 - width;
            if (image.bits() == 24) {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  *out++ = (((0xff00 | (src[2] & 0xf8)) << 5 | (src[1] & 0xf8))
                            << 2) |
                           src[0] >> 3;
                  src += step;
                }
                src += row_skip;
                out += pad;
              }
            } else {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  *out++ = (((((src[3] & 0x80) << 1 | (src[2] & 0xf8)) << 5) |
                             (src[1] & 0xf8))
                            << 2) |
                           src[0] >> 3;
                  src += step;
                }
                src += row_skip;
                out += pad;
              }
            }
            break;
          }
          case k8888: {
            int pad = pitch_ - width * 4;
            if (image.bits() == 24) {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  dest[0] = src[0];
                  dest[1] = src[1];
                  dest[2] = src[2];
                  dest[3] = 0xff;
                  src += step;
                  dest += 4;
                }
                src += row_skip;
                dest += pad;
              }
            } else {
              for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                  dest[0] = src[0];
                  dest[1] = src[1];
                  dest[2] = src[2];
                  dest[3] = src[3];
                  src += step;
                  dest += 4;
                }
                src += row_skip;
                dest += pad;
              }
            }
            break;
          }
          case k8880: {
            int pad = pitch_ - width * 4;
            for (int y = 0; y < height; ++y) {
              for (int x = 0; x < width; ++x) {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = 0;
                src += step;
                dest += 4;
              }
              src += row_skip;
              dest += pad;
            }
            break;
          }
        }
        if (file_name_ == NULL) {
          file_name_ = static_cast<char*>(malloc(strlen(file) + 1));
          if (file_name_ == NULL) {
            return 1;
          }
          strcpy(file_name_, file);
        }
        Log("[%d]ﾌｧｲﾙﾛｰﾄﾞ:%s (%dx%d %dbit)", id_, file, image.width(),
            image.height(), image.bits());
        result = 1;
      }
    }
  }
  Unlock();
  return result;
}
