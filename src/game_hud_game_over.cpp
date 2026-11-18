// GameHud: the game over screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include "enm_param.h"
#include "hls_txt.h"

// 0x420250
int GameHud::Lerp(int* out, int from, int to, int frames, int time) {
  if (time >= frames) {
    *out = to;
    return 1;
  }
  *out = (to - from) * time / frames + from;
  return 0;
}

// 0x420290
int GameHud::InitChart() {
  chart_center_.x = chart_center_.y = chart_center_.z = chart_[0].z =
      chart_[1].z = chart_[2].z = chart_[3].z = chart_[4].z = chart_[5].z =
          chart_[6].z = 0.0f;
  return 1;
}

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

// 0x420330
// TODO: nonmatching: the original does each corner in turn (no loads moved
// ahead of the stores before them) and addresses chart_ off `this`.
void GameHud::UpdateChart() {
  for (int i = 0; i < 6; ++i) {
    float scale = shown_params_[i] * 0.0625f;
    chart_[i].x = g_hexagon[i].x * scale;
    chart_[i].y = g_hexagon[i].y * scale;
  }
  chart_[6] = chart_[0];
}

// 0x420480
void HLSTxt::Measure(SIZE* size) {
  if (text_ != NULL) {
    system_->Measure(size, text_);
  } else {
    size->cx = size->cy = 0;
  }
}

// 0x4204b0
void GameHud::LoadParams() {
  param_count_ = g_enm_param.GetParams(params_);
  for (int i = 0; i < param_count_; ++i) {
    if (params_[i] < 0x40) {
      params_[i] = 0x40;
    }
    shown_params_[i] = 0;
  }
  shown_params_[0] = params_[0];
  UpdateChart();
}
