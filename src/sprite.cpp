// Sprite (Sprite::SetColor() is in cglobal.cpp, whose functions inline it).
#include "sprite.h"

#include <string.h>

#include "application.h"
#include "hl_3d.h"

// 0x41baa0
int Sprite::Initialize() {
  if (app_ == NULL) {
    return 0;
  }
  memset(vertices_, 0, sizeof(vertices_));
  vertices_[0].x = vertices_[2].x = 0.0f;
  vertices_[1].x = vertices_[3].x = app_->width;
  vertices_[0].y = vertices_[1].y = 0.0f;
  vertices_[2].y = vertices_[3].y = app_->height;
  vertices_[0].z = vertices_[1].z = vertices_[2].z = vertices_[3].z = 1e-6f;
  vertices_[0].rhw = vertices_[1].rhw = vertices_[2].rhw = vertices_[3].rhw =
      1.0f;
  vertices_[0].specular = vertices_[1].specular = vertices_[2].specular =
      vertices_[3].specular = 0xffffffff;
  SetColor(0, 0, 0, 0);
  fade_time_ = 0;
  fade_frames_ = 0;
  return 1;
}

// 0x41bb60
int Sprite::IsProtected() {
  if (fade_frames_ != 0) {
    ++fade_time_;
    if (fade_time_ >= fade_frames_) {
      red_ = from_red_ + delta_red_;
      green_ = from_green_ + delta_green_;
      blue_ = from_blue_ + delta_blue_;
      alpha_ = from_alpha_ + delta_alpha_;
      fade_time_ = 0;
      fade_frames_ = 0;
    } else {
      red_ = from_red_ + delta_red_ * fade_time_ / fade_frames_;
      green_ = from_green_ + delta_green_ * fade_time_ / fade_frames_;
      blue_ = from_blue_ + delta_blue_ * fade_time_ / fade_frames_;
      alpha_ = from_alpha_ + delta_alpha_ * fade_time_ / fade_frames_;
    }
    unsigned long c = color();
    vertices_[3].color = c;
    vertices_[2].color = c;
    vertices_[1].color = c;
    vertices_[0].color = c;
  }
  return 1;
}

// 0x41bca0
void Sprite::FadeTo(short frames, int r, int g, int b, int a) {
  from_red_ = red_;
  fade_time_ = 0;
  delta_red_ = r - red_;
  from_green_ = green_;
  delta_green_ = g - green_;
  from_blue_ = blue_;
  delta_blue_ = b - blue_;
  from_alpha_ = alpha_;
  delta_alpha_ = a - alpha_;
  fade_frames_ = frames;
}

// 0x41bd30
Sprite::Sprite() {
  enabled_ = 1;
  app_ = NULL;
}

// 0x41bd60
void Sprite::Run() {
  if (alpha_ != 0) {
    app_->d3d->ClearTexture();
    app_->d3d->SetAlphaBlend(D3DBLEND_INVSRCALPHA);
    app_->d3d->DrawPrimitive(D3DPT_TRIANGLESTRIP,
                             reinterpret_cast<D3DTLVERTEX*>(vertices_), 4);
  }
}
