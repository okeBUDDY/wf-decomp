// GameHud: the name entry screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include "hls_txt.h"

// 0x41f5a0
void GameHud::InitNameEntry() {
  mode_texts_[0] = NULL;
  mode_texts_[1] = NULL;
  mode_texts_[2] = NULL;
  mode_texts_[3] = NULL;
}

// 0x41f5b0
void GameHud::ReleaseNameEntry() {
  if (mode_texts_[0] != NULL) {
    mode_texts_[0]->Release();
    mode_texts_[0] = NULL;
  }
  if (mode_texts_[1] != NULL) {
    mode_texts_[1]->Release();
    mode_texts_[1] = NULL;
  }
  if (mode_texts_[2] != NULL) {
    mode_texts_[2]->Release();
    mode_texts_[2] = NULL;
  }
  if (mode_texts_[3] != NULL) {
    mode_texts_[3]->Release();
    mode_texts_[3] = NULL;
  }
}
