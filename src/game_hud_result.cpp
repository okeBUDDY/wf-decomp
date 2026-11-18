// GameHud: the boss result screen. A unit of its own, so GameHud does not
// inline it.
#include "game_hud.h"

#include "hls_txt.h"

// 0x41fef0
void GameHud::InitResult() { mode_texts_[0] = NULL; }

// 0x41ff00
void GameHud::ReleaseResult() {
  if (mode_texts_[0] != NULL) {
    mode_texts_[0]->Release();
    mode_texts_[0] = NULL;
  }
}
