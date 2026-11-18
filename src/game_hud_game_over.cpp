// GameHud: the game over screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include "hls_txt.h"

// 0x4202e0
void GameHud::InitGameOver() {
  mode_texts_[0] = mode_texts_[1] = NULL;
  mode_texts_[2] = NULL;
}

// 0x4202f0
void GameHud::ReleaseGameOver() {
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
}
