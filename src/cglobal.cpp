// cGlobal: construction.
#include "cglobal.h"

#include "game.h"

// 0x4010a0
cGlobal::cGlobal() {
  music_volume = effect_volume = 1.0f;
  name_ = "cGlobal";
  g_boss = g_player = NULL;
}
