#include "hl_surface.h"

#include <stdlib.h>
#include <string.h>

#include <d3d.h>

#include "application.h"
#include "graphics.h"
#include "hl_image.h"
#include "hl_window.h"

Graphics* HLSurface::graphics_;         // 0x468f70
DDPIXELFORMAT HLSurface::formats_[3];   // 0x468f10
short HLSurface::format_codes_[3];      // 0x468f80
unsigned int HLSurface::texture_caps_;  // 0x468f74
short HLSurface::max_texture_width_;    // 0x468f78
short HLSurface::max_texture_height_;   // 0x468f7c
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

// 0x40b780
void HLSurface::Print(int x, int y, const char* text, COLORREF color,
                      const char* face, int height, int bold, int italic,
                      RECT* rect) {
  HDC dc;
  if (surface_ == NULL || surface_->GetDC(&dc) != DD_OK) {
    return;
  }
  SetBkMode(dc, TRANSPARENT);
  HFONT font;
  if (face != NULL) {
    font = CreateFontA(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic, 0,
                       0, SHIFTJIS_CHARSET, 0, 0, 0, 0, face);
  } else {
    font = graphics_->app_->window_system->fixed_font();
  }
  HGDIOBJ old_font = SelectObject(dc, font);
  SetTextColor(dc, color);
  TextOutA(dc, x, y, text, strlen(text));
  if (rect != NULL) {
    SIZE size;
    GetTextExtentPoint32A(dc, text, strlen(text), &size);
    SetRect(rect, x, y, x + size.cx, y + size.cy);
  }
  SelectObject(dc, old_font);
  if (face != NULL) {
    DeleteObject(font);
  }
  surface_->ReleaseDC(dc);
}

// 0x40b920
HLSurface::HLSurface() {
  surface_ = NULL;
  locked_bits_ = NULL;
  width_ = height_ = pitch_ = format_ = mode_ = 0;
  file_name_ = NULL;
  name_ = "  HLSurface";
  scale_x_ = scale_y_ = 1;
  unknown_18_ = 0;
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

// 0x40bb10
// TODO: nonmatching: the saved width_ and height_ swap stack slots, and the
// Log() arguments and the vtable loads use other registers.
int HLSurface::InitTextureFormats(IDirect3DDevice7* device) {
  // The format search keeps its state in width_ (the kind of
  // format_order_) and height_ (the best index so far).
  short width = width_;
  short height = height_;
  int result = 0;
  D3DDEVICEDESC7 device_desc;
  ZeroMemory(&device_desc, sizeof(device_desc));
  if (device->GetCaps(&device_desc) == D3D_OK) {
    DDCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    caps.dwSize = sizeof(caps);
    if (graphics_->ddraw_->GetCaps(&caps, NULL) == DD_OK) {
      texture_caps_ = 0;
      max_texture_width_ = static_cast<short>(device_desc.dwMaxTextureWidth);
      max_texture_height_ = static_cast<short>(device_desc.dwMaxTextureHeight);
      if (max_texture_width_ == 0) {
        max_texture_width_ = 0x100;
      }
      if (max_texture_height_ == 0) {
        max_texture_height_ = 0x100;
      }
      if (!(caps.dwCaps2 & DDCAPS2_WIDESURFACES) &&
          max_texture_width_ > graphics_->primary_.width_) {
        max_texture_width_ = graphics_->primary_.width_;
      }
      DWORD texture_caps = device_desc.dpcTriCaps.dwTextureCaps;
      if (texture_caps & D3DPTEXTURECAPS_POW2) {
        for (width_ = 1; width_ * 2 <= max_texture_width_; width_ <<= 1) {
        }
        max_texture_width_ = width_;
        for (height_ = 1; height_ * 2 <= max_texture_height_; height_ <<= 1) {
        }
        max_texture_height_ = height_;
        texture_caps_ |= kPow2;
      }
      if (texture_caps & D3DPTEXTURECAPS_SQUAREONLY) {
        if (max_texture_width_ > max_texture_height_) {
          max_texture_width_ = max_texture_height_;
        } else {
          max_texture_height_ = max_texture_width_;
        }
        texture_caps_ |= kSquareOnly;
      }
      if (!(device_desc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_ONE)) {
        texture_caps_ |= kNoBlendOne;
      }
      // Opaque textures: kind 0 or 1 by the 16-bit screen format, else 2.
      width_ = graphics_->app_->depth == 16 ? format_codes_[0] != k555 : 2;
      height_ = kFormatsPerKind - 1;
      if (device->EnumTextureFormats(EnumTextureFormatProc, this) == D3D_OK) {
        // Alpha textures: kind 3, 4 or 5.
        width_ += 3;
        height_ = kFormatsPerKind - 1;
        if (device->EnumTextureFormats(EnumTextureFormatProc, this) == D3D_OK &&
            format_codes_[1] != 0 && format_codes_[2] != 0) {
          // "texture caps Max%dx%d CapsFlag=%08X no alpha:%d alpha:%d"
          Log("ﾃｸｽﾁｬ能力 Max%dx%d CapsFlag=%08X ｱﾙﾌｧ無:%d ｱﾙﾌｧ有:%d",
              max_texture_width_, max_texture_height_, texture_caps_,
              format_codes_[1], format_codes_[2]);
          result = 1;
        }
      }
    }
  }
  height_ = height;
  width_ = width;
  return result;
}

// 0x40bda0
int HLSurface::Create(int width, int height, unsigned short mode) {
  if (graphics_ == NULL || surface_ != NULL) {
    // "cannot make the surface"
    Error("ｻｰﾌｪｽが生成できません");
    return 0;
  }
  mode_ = mode & kKindMask;
  if (mode_ != kTexture) {
    mode_ = kOffscreen;
  }
  DDSURFACEDESC2 desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwWidth = width;
  desc.dwSize = sizeof(desc);
  desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  scale_y_ = 1;
  scale_x_ = 1;
  desc.dwHeight = height;
  if (mode_ == kTexture) {
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    format_ = (mode & kAlpha) != 0 ? 2 : 1;
    desc.ddpfPixelFormat = formats_[format_];
    if (graphics_->app_->flags & AppSettings::kNoHardware3D) {
      mode |= kSystemMemory;
    }
    if (texture_caps_ & kPow2) {
      for (width = 1; width < static_cast<int>(desc.dwWidth); width <<= 1) {
      }
      for (height = 1; height < static_cast<int>(desc.dwHeight); height <<= 1) {
      }
    }
    while (width > max_texture_width_) {
      scale_x_ <<= 1;
      width >>= 1;
    }
    while (height > max_texture_height_) {
      scale_y_ <<= 1;
      height >>= 1;
    }
  } else {
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    format_ = 0;
    if (mode & kRenderTarget) {
      desc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;
      mode_ |= kRenderTarget;
    }
  }
  if (mode & kSystemMemory) {
    desc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    mode_ |= kSystemMemory;
  }
  desc.dwHeight = height;
  desc.dwWidth = width;
  if (graphics_->ddraw_->CreateSurface(&desc, &surface_, NULL) != DD_OK) {
    // "making the surface failed"
    Error("ｻｰﾌｪｽの生成に失敗しました");
    return 0;
  }
  for (id_ = 0; graphics_->surfaces_[id_] != NULL;) {
    if (++id_ == kMaxSurfaces) {
      id_ = -1;
      Release();
      // "too many surfaces"
      Error("ｻｰﾌｪｽ数が多すぎます");
      return 0;
    }
  }
  graphics_->surfaces_[id_] = this;
  Attach(0);
  return 1;
}

// 0x40bff0
// TODO: nonmatching: the pixel conversions match, but the original keeps
// `file` in ebx and `this` in edi (spilled), and keeps the loop counters on
// the stack differently (0x33 bytes longer). One `pad` shared by all cases
// (184 -> 172 differing instructions); shared x/y/out locals do not help.
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
        int pad;
        switch (format_codes_[format_]) {
          case k888: {
            pad = pitch_ - width * 3;
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
            pad = pitch_ / 2 - width;
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
            pad = pitch_ / 2 - width;
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
            pad = pitch_ / 2 - width;
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
            pad = pitch_ / 2 - width;
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
            pad = pitch_ - width * 4;
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
            pad = pitch_ - width * 4;
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
