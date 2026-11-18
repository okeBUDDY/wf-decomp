// HLCamera: the camera task HL3D::Initialize() puts in Application::overlays
// (vtable 0x4517e8, "HLCamera"). It sets the view transform of the device
// when it moved; the game never enables it, so its debug text (Run()) is
// never drawn.
#ifndef WF_HL_CAMERA_H_
#define WF_HL_CAMERA_H_

#include <windows.h>

#include <d3d.h>

#include "task_list.h"

class HL3D;

class HLCamera : public Task {
 public:
  enum Flags {
    kMoved = 0x1,  // set the view transform
    kOrbit = 0x2,  // place the eye by the rotation around the target first
  };

  HLCamera();  // 0x403da0

  virtual int Initialize();   // 0x403fd0
  virtual int IsProtected();  // 0x403dd0
  virtual void Run();         // 0x403c30

  // Moves the eye to `position`, looking at the target.
  void SetPosition(const D3DVECTOR* position);  // 0x403f50
  // Moves the target to `target`.
  void SetTarget(const D3DVECTOR* target) {
    target_.x = target->x;
    target_.y = target->y;
    target_.z = target->z;
    flags_ |= kMoved;
  }

  static HL3D* d3d_;  // 0x468b88

 private:
  unsigned char flags_;    // 0x14
  D3DVECTOR home_eye_;     // 0x18
  D3DVECTOR home_target_;  // 0x24
  float home_roll_;        // 0x30
  D3DVECTOR eye_;          // 0x34
  D3DVECTOR target_;       // 0x40
  D3DVECTOR rotation_;     // 0x4c, radians about x, y and z
  float distance_;         // 0x58, from the eye to the target
};

#endif  // WF_HL_CAMERA_H_
