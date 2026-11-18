#include "hl_surface.h"

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
