// GameHud: the boss result screen. A unit of its own, so GameHud does not
// inline it.
#include "game_hud.h"

#include "application.h"
#include "enm_param.h"
#include "fonts.h"
#include "hl_3d.h"
#include "hls_txt.h"
#include "snd_sys.h"
#include "sounds.h"

// 0x41fef0
void GameHud::InitResult() { mode_texts_[0] = NULL; }

// 0x41ff00
void GameHud::ReleaseResult() {
  if (mode_texts_[0] != NULL) {
    mode_texts_[0]->Release();
    mode_texts_[0] = NULL;
  }
}

// The bar behind the result, scrolling, and its texels in font2.png.
static D3DVECTOR kResultBar[4] = {
    {0, -8, 0}, {768, -8, 0}, {0, 8, 0}, {768, 8, 0}};  // 0x45dd04
static POINTS kResultBarTexels[4] = {
    {0, 0}, {768, 0}, {0, 16}, {768, 16}};  // 0x45dd34

// 0x41ff20
int GameHud::MoveResult() {
  int step = step_;
  switch (step) {
    case 0: {
      mode_texts_[0] =
          g_fonts.small_text.Create(g_enm_param.GetResultSize(), NULL);
      if (mode_texts_[0] == NULL) {
        return 1;
      }
      int columns;
      int lines;
      g_enm_param.FormatResult(mode_texts_[0]->text(), &columns, &lines);
      result_bar_ = 0;
      result_alpha_ = 1.0f;
      result_shown_ = 0;
      result_length_ = lines * columns;
      result_height_ = lines * 16;
      ++step_;
      break;
    }
    case 1:
      if (mode_time_ > 30) {
        step_ = 2;
      }
      break;
    case 2:
      if (++result_shown_ > result_length_) {
        flags_ &= ~kFlagSkip;
        step_ = 3;
      }
      break;
    case 3:
      if (Lerp(&result_bar_, 0, 1.0f, 10, mode_time_)) {
        step_ = 10;
      }
      break;
    case 10:
      if (flags_ & kFlagSkip) {
        g_application.sound->channel(kSndSwitch)
            ->Assign(&g_sounds[kSndSwitch], 0, 1.0f);
        step_ = 20;
      }
      break;
    case 20:
      Lerp(&result_alpha_, 1.0f, 0, 10, mode_time_);
      if (Lerp(&result_bar_, 1.0f, 0, 10, mode_time_)) {
        ++step_;
      }
      break;
    case 21:
      if (mode_time_ > 30) {
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

// 0x4200e0
void GameHud::DrawResult() {
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
  ScaleTransform(&transform, 1.0f, result_bar_);
  TranslateTransform(
      &transform,
      -static_cast<float>(
          static_cast<unsigned int>(g_application.frame_count & 0x3f) * 2),
      460);
  SetTexture(g_application.d3d, &g_fonts.textures[2], 0);
  g_application.d3d->SetFilter(result_bar_ != 1.0f, 0);
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &transform, kResultBar,
                                kResultBarTexels, 0xc800ff80, 4);
  g_application.d3d->SetPointFilter(0);
  D3DCOLOR palette[2];
  palette[0] = static_cast<int>(result_alpha_ * 128) << 24 | 0xffffff;
  palette[1] = static_cast<int>(result_alpha_ * 200) << 24 | 0xffc800;
  mode_texts_[0]->Print(16,
                        448 - result_height_ * result_shown_ / result_length_,
                        palette, result_shown_);
}
