// HLSTxt::Print(). The original has them in the objects of the option
// (0x41ca70) and title (0x41dfd0) screens.
#include "application.h"
#include "hl_surface.h"
#include "hls_txt.h"

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
