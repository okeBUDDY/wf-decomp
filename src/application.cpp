#include "application.h"

#include <stdlib.h>

#include "system.h"
#include "user_func.h"

// 0x406890
int Application::AddSystem(System* system, bool owns) {
  if (system == NULL) {
    return 0;
  }
  if (system_count == kMaxSystems) {
    // "too many systems registered"
    Error(
        "\274\275\303\321\202\314\223o\230^"
        "\220\224\202\252\221\275\202\267\202\254\202\334\202\267");
    return 0;
  }
  systems[system_count] = system;
  owned[system_count] = owns;
  ++system_count;
  system->Log("Init");
  return system->Initialize(this);
}

// 0x406b80
void Application::Shutdown() {
  if (last_error()[0] != '\0') {
    MessageBox(window, last_error(), "ERROR", MB_ICONWARNING);
  }
  tasks.RemoveFrom(0);
  overlays.RemoveFrom(0);
  for (int i = system_count - 1; i >= 0; --i) {
    systems[i]->Finalize();
    systems[i]->Log("End");
    if (owned[i] && systems[i] != NULL) {
      delete systems[i];
    }
    systems[i] = NULL;
    owned[i] = false;
  }
  system_count = 0;
}

// 0x4067d0
void Application::DispatchMsg(MSG* msg) {
  TranslateMessage(msg);
  DispatchMessage(msg);
}

// 0x4067f0
void Application::Pause(int pause) {
  if (pause) {
    if (!(flags & kPaused)) {
      for (int i = 0; i < system_count; ++i) {
        systems[i]->Pause();
      }
      flags |= kPaused;
    }
  } else if (flags & kPaused) {
    flags = (flags & ~kPaused) | kResumed;
  }
}

// 0x4071b0
void Application::Quit() {
  Log("AllEnd");
  if (flags & kRunning) {
    user_func->Finish(NULL);
    Pause(0);
    flags &= ~kSkipFrame;
  } else {
    Shutdown();
    exit(0);
  }
}
