#include "sprite.h"

// 0x401000
void Sprite::SetColor(int r, int g, int b, int a) {
  red_ = r;
  green_ = g;
  blue_ = b;
  alpha_ = a;
  unsigned long color = (((((alpha_ << 8) | red_) << 8) | green_) << 8) | blue_;
  vertices_[3].color = color;
  vertices_[2].color = color;
  vertices_[1].color = color;
  vertices_[0].color = color;
  color_flag_b0_ = 0;
}
