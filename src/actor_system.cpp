#include "actor_system.h"

#include "actor_control.h"

// 0x407980
int ActorSystem::Register(int id, ActorControl* control) {
  if (controls_ != NULL && control != NULL) {
    if (id <= 0) {
      // "Invalid control system id"
      Error(
          "\225s\220\263\202\310\220\247\214\344\274\275\303\321ID\202\305"
          "\202\267");
      return 0;
    }
    if (!control->Initialize()) {
      return 0;
    }
    if (controls_[id] != NULL) {
      controls_[id]->Finalize();
      delete controls_[id];
    }
    controls_[id] = control;
    return 1;
  }
  // "Out of memory"
  Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
  return 0;
}
