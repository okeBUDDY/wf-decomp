// HLInput: DirectInput keyboard and joystick polling (vtable 0x451a50).
#ifndef WF_HL_INPUT_H_
#define WF_HL_INPUT_H_

#include "system.h"

struct Application;

class HLInput : public System {
 public:
  HLInput();                                 // 0x405e40
  virtual int Initialize(Application* app);  // 0x406530
  virtual void Finalize();                   // 0x406750
  virtual void Pause() {}
  virtual void Resume() {}

 private:
  char unknown_08_[0x44];
};

#endif  // WF_HL_INPUT_H_
