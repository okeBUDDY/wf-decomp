// GameHud: the game over screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include <stdio.h>
#include <string.h>

#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "fonts.h"
#include "hl_3d.h"
#include "hls_txt.h"
#include "snd_sys.h"
#include "sounds.h"

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

// 0x420bd0
// TODO: nonmatching: the original pushes Create()'s arguments before it
// picks the title font (and stores mode_texts_[2] before the NULL checks);
// the rest matches.
int GameHud::MoveGameOver() {
  int step = step_;
  switch (step) {
    case 0: {
      mode_texts_[0] = g_text_font.Create(16, NULL);
      mode_texts_[1] = g_text_font.Create(16, NULL);
      mode_texts_[2] =
          (g_global.flags & cGlobal::kFlagEnglish ? g_title_font : g_kana_font)
              .Create(0x80, NULL);
      if (mode_texts_[0] == NULL || mode_texts_[1] == NULL ||
          mode_texts_[2] == NULL) {
        return 1;
      }
      LoadParams();
      frame_open_ = 0;
      band_height_ = 0;
      info_alpha_ = 1.0f;
      chart_alpha_ = 0;
      title_slide_ = 400.0f;
      const char* name = "NAME:";
      strcpy(mode_texts_[0]->text(), name);
      sprintf(mode_texts_[1]->text(), "STAGE:%02d", g_global.stage);
      if (g_global.flags & cGlobal::kFlagEnglish) {
        g_enm_param.FormatTitleEn(mode_texts_[2]->text());
      } else {
        g_enm_param.FormatTitleJa(mode_texts_[2]->text());
      }
      mode_texts_[2]->Measure(&title_size_);
      title_pos_.x = static_cast<int>(title_slide_) + 360;
      title_pos_.y = 400 - title_size_.cy;
      g_application.sound->channel(kSndWarning)
          ->Assign(&g_sounds[kSndWarning], 0, 1.0f);
      ++step_;
      break;
    }
    case 1: {
      int angle;
      if (Lerp(&angle, 0, 0x100, 20, mode_time_)) {
        step_ = 2;
      }
      frame_open_ = g_sin_table[angle & 0x3ff];
      break;
    }
    case 2:
      if (Lerp(&band_height_, 0, 78.0f, 20, mode_time_)) {
        ++step_;
      }
      break;
    case 3:
      if (Lerp(&chart_alpha_, 0, 1.0f, 10, mode_time_)) {
        flags_ &= ~kFlagSkip;
        ++step_;
      }
      break;
    case 4: {
      // Grow the chart one corner at a time.
      int done = 0;
      for (int i = 0; i < param_count_; ++i) {
        if (shown_params_[i] < params_[i]) {
          shown_params_[i] += 0x40;
          if (shown_params_[i] > params_[i]) {
            shown_params_[i] = params_[i];
          }
          UpdateChart();
          if (i < 6) {
            break;
          }
        } else {
          ++done;
        }
      }
      int angle;
      if (Lerp(&angle, 0, 0x100, 20, mode_time_) && done >= param_count_) {
        step_ = 10;
      }
      title_slide_ = (1.0f - g_sin_table[angle & 0x3ff]) * 400.0f;
      break;
    }
    case 10:
      if (mode_time_ >= 180 || (flags_ & kFlagSkip)) {
        step_ = 20;
      }
      break;
    case 20:
      Lerp(&chart_alpha_, 1.0f, 0, 20, mode_time_);
      if (Lerp(&info_alpha_, 1.0f, 0, 20, mode_time_)) {
        ++step_;
      }
      break;
    case 21:
      Lerp(&title_slide_, 0, 400.0f, 20, mode_time_);
      Lerp(&frame_open_, 1.0f, 0, 20, mode_time_);
      if (Lerp(&band_height_, 78.0f, 0, 20, mode_time_)) {
        return 1;
      }
      break;
  }
  if (step != step_) {
    mode_time_ = 0;
  } else {
    ++mode_time_;
  }
  return 0;
}
