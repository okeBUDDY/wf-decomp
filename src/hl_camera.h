// HLCamera: the camera task HL3D::Initialize() puts in Application::overlays
// (vtable 0x4517e8, "HLCamera"). Only what HL3D uses is known so far.
#ifndef WF_HL_CAMERA_H_
#define WF_HL_CAMERA_H_

#include "task_list.h"

class HL3D;

class HLCamera : public Task {
 public:
  HLCamera();  // 0x403da0

  static HL3D* d3d_;  // 0x468b88

 private:
  char unknown_14_[0x5c - 0x14];
};

#endif  // WF_HL_CAMERA_H_
