// HLCamera::Initialize, apart from SetPosition(): the original does not
// assume SetPosition() keeps ecx, which VC7.1 does within one file.
#include "hl_camera.h"

// 0x403fd0
int HLCamera::Initialize() {
  home_eye_.x = 0;
  home_eye_.y = 0;
  home_eye_.z = -500;
  home_target_.x = 0;
  home_target_.y = 0;
  home_target_.z = 0;
  home_roll_ = 0;
  SetTarget(&home_target_);
  SetPosition(&home_eye_);
  rotation_.z = home_roll_;
  enabled_ = 0;
  flags_ = 0;
  return 1;
}
