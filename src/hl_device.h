// HLDevice: the system at Application+0xa0 (vtable 0x451848). Only its
// size is known so far.
#ifndef WF_HL_DEVICE_H_
#define WF_HL_DEVICE_H_

#include "system.h"

struct Application;

class HLDevice : public System {
 public:
  HLDevice();                                // 0x404840
  virtual ~HLDevice();                       // 0x404910
  virtual int Initialize(Application* app);  // 0x404f90
  virtual void Finalize();                   // 0x404350
  virtual void Pause() {}
  virtual void Resume() {}

 private:
  char unknown_08_[0x58];
};

#endif  // WF_HL_DEVICE_H_
