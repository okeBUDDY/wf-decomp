// System: a service the Application initializes at startup and finalizes
// at shutdown (window, timer, sound, input, ...).
#ifndef WF_SYSTEM_H_
#define WF_SYSTEM_H_

#include "object.h"

struct Application;

class System : public Object {
 public:
  // Returns 0 when the system cannot run.
  virtual int Initialize(Application* app) = 0;
  virtual void Finalize() = 0;
  // Called while the application is paused / when it resumes.
  virtual void Pause() = 0;
  virtual void Resume() = 0;
};

#endif  // WF_SYSTEM_H_
