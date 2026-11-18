// HLSTxt::Print(). The original has them in the objects of the option
// (0x41ca70) and title (0x41dfd0) screens.
#include "application.h"
#include "hl_surface.h"
#include "hls_txt.h"

// HL3D::SetTexture() (0x407880) as these functions inline it.
inline void SetTexture(HL3D* d3d, HLSurface* texture, int stage) {
  if (d3d->texture_ != texture) {
    if (texture == NULL) {
      d3d->device_->SetTexture(stage, NULL);
    } else {
      d3d->device_->SetTexture(stage, texture->surface());
    }
    d3d->texture_ = texture;
  }
}

inline void HLSTxt::BindTexture() {
  SetTexture(system_->app_->d3d, texture_, 0);
}

// 0x41ca70
void HLSTxt::Print(int x, int y, D3DCOLOR color, int count) {
  BindTexture();
  system_->Print(static_cast<float>(x), static_cast<float>(y), text_, NULL,
                 color, count);
}

// 0x41dfd0
void HLSTxt::Print(int x, int y, const D3DCOLOR* palette, int count) {
  BindTexture();
  system_->Print(static_cast<float>(x), static_cast<float>(y), text_, palette,
                 palette[0], count);
}
