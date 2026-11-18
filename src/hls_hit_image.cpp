// The hit image unit, with a few HLSurface and HL3D helpers.
#include "hls_hit_image.h"

#include <stdlib.h>

#include "hl_3d.h"
#include "hl_surface.h"

// 0x407650
int HLSHitImage::Create(int width, int height) {
  shift_ = 0;
  if (height > kMaxHeight) {
    int shift = 0;
    do {
      width >>= 1;
      height >>= 1;
      ++shift;
    } while (height > kMaxHeight);
    shift_ = shift;
  }
  width_ = width;
  height_ = height;
  size_ = width * height * 2;
  bits_ = static_cast<unsigned short*>(malloc(size_));
  if (bits_ == NULL) {
    // "Out of memory."
    Error("ﾒﾓﾘが足りません.");
    return 0;
  }
  // "Allocated the (%dx%d) hit image"
  Log("(%dx%d)のﾋｯﾄｲﾒｰｼﾞを確保", width, height);
  return 1;
}

// 0x4076e0
void HLSHitImage::Release() {
  if (surface_ != NULL) {
    surface_->Release();
    delete surface_;
    surface_ = NULL;
  }
  if (bits_ != NULL) {
    free(bits_);
  }
  bits_ = NULL;
}

// 0x407720
void HLSHitImage::Fill(int x, int y, int width, int height, unsigned long id) {
  // The center and half sizes become the top left corner and sizes.
  x = (x - width) >> shift_;
  y = (y - height) >> shift_;
  width = (width >> shift_) * 2;
  height = (height >> shift_) * 2;
  if (x + width <= 0 || x >= width_ || y + height <= 0 || y >= height_) {
    return;
  }
  if (x < 0) {
    width = x + width;
    x = 0;
  }
  if (y < 0) {
    height = y + height;
    y = 0;
  }
  if (x + width >= width_) {
    width = width_ - x;
  }
  if (y + height >= height_) {
    height = height_ - y;
  }
  if (width == 0 || height == 0) {
    return;
  }
  unsigned short* bits = bits_ + width_ * y + x;
  int skip = (width_ - width) * 2;
  id |= id << 16;
  __asm {
    push edi
    push esi
    mov edi, bits
    mov ebx, height
    mov edx, width
    mov esi, skip
    mov eax, id
    cld
  row:
    mov ecx, edx
    rep stosw
    add edi, esi
    dec ebx
    jne row
    pop esi
    pop edi
  }
}

// 0x407820
void* HLSurface::Lock() {
  if (surface_ == NULL) {
    return NULL;
  }
  DDSURFACEDESC2 desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwSize = sizeof(desc);
  if (surface_->Lock(NULL, &desc, DDLOCK_WAIT, NULL) != DD_OK) {
    return NULL;
  }
  pitch_ = static_cast<short>(desc.lPitch);
  locked_bits_ = desc.lpSurface;
  return locked_bits_;
}

// 0x407880
void HL3D::SetTexture(HLSurface* texture, int stage) {
  if (texture_ != texture) {
    if (texture == NULL) {
      device_->SetTexture(stage, NULL);
    } else {
      device_->SetTexture(stage, texture->surface_);
    }
    texture_ = texture;
  }
}

// 0x4078d0
HLSHitImage::HLSHitImage() {
  bits_ = NULL;
  surface_ = NULL;
  name_ = "HLSHitImage";
}
