#include "hl_camera.h"

#include <math.h>
#include <stdio.h>

#include "application.h"
#include "hl_3d.h"

static char kCameraName[] = "HLCamera";  // 0x45d1a0

HL3D* HLCamera::d3d_;  // 0x468b88

// 0x403c30
void HLCamera::Run() {
  int bottom = d3d_->app_->height;
  sprintf(g_text_buffer, "Cam (%8.2f,%8.2f,%8.2f)", eye_.x, eye_.y, eye_.z);
  d3d_->app_->screen->Print(0, bottom - 64, g_text_buffer, 0xffffff, NULL, 16,
                            0, 0, NULL);
  sprintf(g_text_buffer, "Tgt (%8.2f,%8.2f,%8.2f)", target_.x, target_.y,
          target_.z);
  d3d_->app_->screen->Print(0, bottom - 48, g_text_buffer, 0xffffff, NULL, 16,
                            0, 0, NULL);
  sprintf(g_text_buffer, "Rot (%8.2f,%8.2f,%8.2f)", rotation_.x * 57.29578f,
          rotation_.y * 57.29578f, rotation_.z * 57.29578f);
  d3d_->app_->screen->Print(0, bottom - 32, g_text_buffer, 0xffffff, NULL, 16,
                            0, 0, NULL);
  sprintf(g_text_buffer, "Dist = %8.2f", distance_);
  d3d_->app_->screen->Print(0, bottom - 16, g_text_buffer, 0xffffff, NULL, 16,
                            0, 0, NULL);
}

// 0x403da0
HLCamera::HLCamera() {
  enabled_ = 0;
  name_ = kCameraName;
}

// 0x403dd0
int HLCamera::IsProtected() {
  if (flags_ != 0) {
    Transform transform;
    if (flags_ & kOrbit) {
      SetIdentity(&transform);
      RotateTransformX(&transform, rotation_.x);
      RotateTransformY(&transform, rotation_.y);
      RotateTransformZ(&transform, rotation_.z);
      TranslateTransform(&transform, target_.x, target_.y);
      eye_.x = eye_.y = eye_.z = 0;
      // TransformPoint() writes sx, sy and sz: the x, y and z of eye_.
      TransformPoint(reinterpret_cast<D3DTLVERTEX*>(&eye_), &transform, &eye_);
    }
    SetIdentity(&transform);
    TranslateTransform(&transform, -target_.x, -target_.y);
    RotateTransformZ(&transform, -rotation_.z);
    RotateTransformY(&transform, -rotation_.y);
    RotateTransformX(&transform, -rotation_.x);
    // A Transform has the layout of a D3DMATRIX.
    d3d_->device_->SetTransform(D3DTRANSFORMSTATE_VIEW,
                                reinterpret_cast<D3DMATRIX*>(&transform));
    flags_ = 0;
  }
  return 1;
}

// 0x403f50
void HLCamera::SetPosition(const D3DVECTOR* position) {
  eye_.x = position->x;
  eye_.y = position->y;
  eye_.z = position->z;
  D3DVECTOR offset;
  offset.x = eye_.x - target_.x;
  offset.y = eye_.y - target_.y;
  offset.z = eye_.z - target_.z;
  rotation_.y = static_cast<float>(atan2(-offset.x, -offset.z));
  float across =
      static_cast<float>(sqrt(offset.z * offset.z + offset.x * offset.x));
  rotation_.x = static_cast<float>(atan2(offset.y, across));
  distance_ = static_cast<float>(sqrt(across * across + offset.y * offset.y));
  flags_ |= kMoved;
}
