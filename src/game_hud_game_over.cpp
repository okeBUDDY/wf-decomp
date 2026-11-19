// GameHud: the game over screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include <stdio.h>
#include <string.h>

#include "application.h"
#include "background.h"
#include "cglobal.h"
#include "enm_param.h"
#include "fonts.h"
#include "game_data.h"
#include "hl_3d.h"
#include "hl_surface.h"
#include "hls_pat.h"
#include "hls_txt.h"
#include "snd_sys.h"
#include "sounds.h"

inline void HLSTxt::BindTexture() {
  SetTexture(system_->app_->d3d, texture_, 0);
}

inline void HLSTxt::PrintInline(int x, int y, D3DCOLOR color, int count) {
  BindTexture();
  system_->Print(static_cast<float>(x), static_cast<float>(y), text_, NULL,
                 color, count);
}

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

// The band behind the frame (etc0.pat).
static D3DVECTOR kBand[4] = {
    {-256, -24, 0}, {768, -24, 0}, {-256, 24, 0}, {768, 24, 0}};  // 0x45dd48
static POINTS kBandTexels[4] = {
    {0, 208}, {1024, 208}, {0, 256}, {1024, 256}};  // 0x45dd78
// The line under the boss's title.
static D3DVECTOR kTitleLine[3] = {
    {-20, 20, 0}, {0, 0, 0}, {360, 0, 0}};  // 0x45dd88
// The weapon icons (etc2.pat) and their texels by weapon.
static D3DVECTOR kWeaponIcon[4] = {
    {-16, 0, 0}, {64, 0, 0}, {-16, 24, 0}, {64, 24, 0}};  // 0x45ddac
static POINTS kWeaponTexels[7][4] = {
    {{96, 128}, {176, 128}, {96, 152}, {176, 152}},
    {{96, 152}, {176, 152}, {96, 176}, {176, 176}},
    {{96, 176}, {176, 176}, {96, 200}, {176, 200}},
    {{96, 200}, {176, 200}, {96, 224}, {176, 224}},
    {{96, 224}, {176, 224}, {96, 248}, {176, 248}},
    {{176, 128}, {256, 128}, {176, 152}, {256, 152}},
    {{176, 152}, {256, 152}, {176, 176}, {256, 176}}};  // 0x45dde0
// The level bar of a weapon, 64 pixels at level 1024.
static D3DVECTOR kWeaponBar[4] = {
    {0, 2, 0}, {64, 2, 0}, {0, 12, 0}, {64, 12, 0}};  // 0x45de50

// 0x420500
void GameHud::DrawGameOver() {
  Transform transform;
  transform.m[2][2] = 1;
  transform.m[1][1] = 1;
  transform.m[0][0] = 1;
  transform.m[1][2] = 0;
  transform.m[1][0] = 0;
  transform.m[0][2] = 0;
  transform.m[0][1] = 0;
  transform.m[2][0] = 0;
  transform.m[2][1] = 0;
  ScaleTransform(&transform, 1.0f, frame_open_);
  TranslateTransform(
      &transform,
      -static_cast<float>(
          static_cast<unsigned int>(g_application.frame_count & 0x3f) * 4),
      -200);
  RotateTransformZ(&transform, 0.52359879f);
  SetTexture(g_application.d3d, g_pats.etc[0].texture(), 0);
  Transform band;
  band.m[0][0] = transform.m[0][0];
  band.m[0][1] = transform.m[0][1];
  band.m[0][2] = transform.m[0][2];
  band.m[1][0] = transform.m[1][0];
  band.m[1][1] = transform.m[1][1];
  band.m[1][2] = transform.m[1][2];
  band.m[2][0] = transform.m[2][0] + 160;
  band.m[2][1] = transform.m[2][1] + 200;
  band.m[2][2] = transform.m[2][2];
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &band, kBand, kBandTexels,
                                0xc8ff4000, 4);
  RotateTransform(&transform, 0x200);
  TranslateTransform(&transform, 160, 200);
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &transform, kBand,
                                kBandTexels, 0xc8ff4000, 4);

  D3DCOLOR chart_color = static_cast<int>(chart_alpha_ * 200) << 24 | 0xff80;
  D3DCOLOR info_color = static_cast<int>(info_alpha_ * 128) << 24 | 0xff8000;
  transform.m[2][2] = 1;
  transform.m[1][1] = 1;
  transform.m[0][0] = 1;
  transform.m[1][2] = 0;
  transform.m[1][0] = 0;
  transform.m[0][2] = 0;
  transform.m[0][1] = 0;
  transform.m[2][0] = 160;
  transform.m[2][1] = 200;
  g_pats.etc[1].Get(1)->Draw(&transform, chart_color, 0);
  g_application.d3d->ClearTexture();
  g_application.d3d->DrawPoints(D3DPT_TRIANGLEFAN, &transform, &chart_center_,
                                info_color, 8);
  if (band_height_ > 0) {
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = 0;
    transform.m[2][1] = 0;
    ScaleTransform(&transform, band_height_, band_height_);
    RotateTransform(&transform, g_application.frame_count * -3);
    TranslateTransform(&transform, 160, 200);
    g_application.d3d->DrawPoints(D3DPT_LINESTRIP, &transform, g_hexagon,
                                  0xc800ff80, 7);
  }

  SetTexture(g_application.d3d, g_pats.etc[2].texture(), 0);
  float x = 360;
  float y = 172;
  for (int i = 0; i < g_enm_param.state_.weapon_count; ++i) {
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = x;
    transform.m[2][1] = y;
    g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &transform, kWeaponIcon,
                                  kWeaponTexels[g_enm_param.state_.weapons[i]],
                                  chart_color, 4);
    x -= 21;
    y += 36;
  }
  g_application.d3d->ClearTexture();
  x = 360;
  y = 172;
  for (int i = 0; i < g_enm_param.state_.weapon_count; ++i) {
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = 0;
    transform.m[2][1] = 0;
    ScaleTransform(&transform, shown_params_[6 + i] / 1024.0f, 1.0f);
    TranslateTransform(&transform, x, y);
    g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, &transform, kWeaponBar,
                                  info_color, 4);
    x -= 21;
    y += 36;
  }

  SetIdentity(&transform);
  TranslateTransform(&transform, title_slide_ + 360, 404);
  title_pos_.x = static_cast<int>(title_slide_) + 360;
  g_application.d3d->DrawPoints(D3DPT_LINESTRIP, &transform, kTitleLine,
                                0xc8ffc800, 3);
  game_over_texts_[0]->PrintInline(title_pos_.x, title_pos_.y - 20, 0xc8ffc800,
                                   -1);
  game_over_texts_[2]->PrintInline(title_pos_.x + 20, title_pos_.y, 0xc8ffc800,
                                   -1);
  game_over_texts_[1]->PrintInline(title_pos_.x + 144, 408, 0xc8ffc800, -1);
}

// 0x420bd0
int GameHud::MoveGameOver() {
  int step = step_;
  switch (step) {
    case 0: {
      mode_texts_[0] = g_fonts.text.Create(16, NULL);
      mode_texts_[1] = g_fonts.text.Create(16, NULL);
      if (g_global.flags & cGlobal::kFlagEnglish) {
        mode_texts_[2] = g_fonts.title.Create(0x80, NULL);
      } else {
        mode_texts_[2] = g_fonts.kana.Create(0x80, NULL);
      }
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
