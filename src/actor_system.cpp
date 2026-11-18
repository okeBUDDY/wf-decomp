#include "actor_system.h"

#include "actor.h"
#include "actor_control.h"

// 0x407900
void Actor::LerpColor(DWORD from, DWORD to, int num, int den) {
  const Color* a = reinterpret_cast<const Color*>(&from);
  const Color* b = reinterpret_cast<const Color*>(&to);
  color.b = a->b + (b->b - a->b) * num / den;
  color.g = a->g + (b->g - a->g) * num / den;
  color.r = a->r + (b->r - a->r) * num / den;
  color.a = a->a + (b->a - a->a) * num / den;
}

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
