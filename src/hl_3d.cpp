#include "hl_3d.h"

// 0x402bb0
HL3D::HL3D() {
  app_ = NULL;
  d3d_ = NULL;
  device_ = NULL;
  unknown_10_ = 0;
  zbuffer_ = NULL;
  unknown_20_ = 0;
  flags_ = 0;
  unknown_70_ = 0;
  unknown_6c_ = 0;
  name_ = "HL3D";
  depth_ = 16;
}
