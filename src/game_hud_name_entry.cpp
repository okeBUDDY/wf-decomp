// GameHud: the name entry screen. A unit of its own, so GameHud does not inline
// it.
#include "game_hud.h"

#include <stdio.h>
#include <string.h>

#include "application.h"
#include "background.h"
#include "cglobal.h"
#include "fn_game.h"
#include "fonts.h"
#include "hl_input.h"
#include "hls_txt.h"
#include "score_table.h"
#include "snd_sys.h"
#include "sounds.h"

// The characters of a name: the letters, digits, signs, '<' (delete) and
// '$' (end), with spaces between the groups. Static, so VC knows no store
// changes it.
static const char* kNameChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 %&+-/!?.,_ <$";  // 0x45dc48

// The scrolling bars above and below the window, and their texels in
// font2.png.
static D3DVECTOR kBarPoints[4] = {
    {-320, -8, 0}, {448, -8, 0}, {-320, 8, 0}, {448, 8, 0}};  // 0x45dc4c
static POINTS kBarTexels[4] = {
    {0, 32}, {768, 32}, {0, 48}, {768, 48}};  // 0x45dc7c
// The window's backdrop.
static D3DVECTOR kBackdrop[4] = {
    {-320, -59, 0}, {320, -59, 0}, {-320, 60, 0}, {320, 60, 0}};  // 0x45dc8c
// The line under the letter being entered.
static D3DVECTOR kNameCursor[2] = {{0, 18, 0}, {16, 18, 0}};  // 0x45dcbc
// The box around the selected character.
static D3DVECTOR kCharCursor[4] = {
    {0, 0, 0}, {8, 0, 0}, {0, 8, 0}, {8, 8, 0}};  // 0x45dcd4

// kNameChars.
enum {
  kCharDigits = 27,
  kCharSigns = 38,
  kCharLastSign = 47,
  kCharDelete = 49,
  kCharEnd = 50,
  kChars = 51,
  kNameLength = 3,
};

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

// 0x41f600
int GameHud::EnterName() {
  HLInpDev* pad = g_application.input->pad(0);
  unsigned short pressed = pad->pressed();
  unsigned short dirs = pad->buttons() & FnGame::kKeyDirections;
  if (last_dirs_ == dirs) {
    ++dirs_time_;
    if (dirs_time_ > 30 && !(dirs_time_ & 3)) {
      pressed |= dirs;
    }
  } else {
    dirs_time_ = 0;
    last_dirs_ = dirs;
  }
  if (pressed & FnGame::kKeyDirections) {
    // No delete or end at the first letter, only those after the last.
    int last;
    int first;
    switch (cursor_) {
      case 0:
        first = 0;
        last = kCharLastSign;
        break;
      case kNameLength:
        first = kCharDelete;
        last = kCharEnd;
        break;
      default:
        first = 0;
        last = kCharEnd;
        break;
    }
    if (pressed & (HLInpDev::kDown | HLInpDev::kRight)) {
      if (++char_index_ > last) {
        char_index_ = first;
      }
      while (kNameChars[char_index_] == ' ') {
        ++char_index_;
      }
    } else {
      if (--char_index_ < first) {
        char_index_ = last;
      }
      while (kNameChars[char_index_] == ' ') {
        --char_index_;
      }
    }
    g_application.sound->channel(kSndFireMe)
        ->Assign(&g_sounds[kSndFireMe], 0, 1.0f);
  } else {
    int index = char_index_;
    if ((pressed & (FnGame::kKeyFire | FnGame::kKeySlow)) == FnGame::kKeySlow) {
      index = kCharDelete;
      pressed |= FnGame::kKeyFire;
    }
    if (pressed & FnGame::kKeyFire) {
      g_application.sound->channel(kSndSwitch)
          ->Assign(&g_sounds[kSndSwitch], 0, 1.0f);
      switch (index) {
        case kCharDelete:
          if (cursor_ > 0) {
            mode_texts_[2]->text()[cursor_] =
                cursor_ >= kNameLength ? ' ' : '-';
            --cursor_;
            for (int i = 0; i < kChars; ++i) {
              if (name_[cursor_] == kNameChars[i]) {
                char_index_ = i;
              }
            }
          }
          break;
        case kCharEnd:
          name_[cursor_] = '\0';
          mode_texts_[2]->text()[cursor_] = '\0';
          return 1;
        default:
          name_[cursor_] = kNameChars[char_index_];
          if (++cursor_ >= kNameLength) {
            char_index_ = kCharEnd;
          }
          break;
      }
    }
  }
  mode_texts_[2]->text()[cursor_] = kNameChars[char_index_];
  return 0;
}

// 0x41fc90
int GameHud::MoveNameEntry() {
  int step = step_;
  switch (step) {
    case 0: {
      int rank = g_score_table.FindRank(&g_global.entry);
      if (rank < 0) {
        return 1;
      }
      mode_texts_[0] = g_fonts.text.Create(0x40, NULL);
      mode_texts_[1] = g_fonts.small_text.Create(0, "PLEASE ENTER YOUR NAME!");
      mode_texts_[2] = g_fonts.text.Create(8, NULL);
      mode_texts_[3] =
          g_fonts.small_text.Create(0, const_cast<char*>(kNameChars));
      if (mode_texts_[0] == NULL || mode_texts_[1] == NULL ||
          mode_texts_[2] == NULL || mode_texts_[3] == NULL) {
        return 1;
      }
      sprintf(mode_texts_[0]->text(), "%s: %012d", kRankNames[rank],
              g_global.entry.score);
      const char* blank = "A-- ";
      strcpy(mode_texts_[2]->text(), blank);
      cursor_ = 0;
      char_index_ = 0;
      back_alpha_ = 0;
      text_alpha_ = 0;
      open_ = 0;
      ++step_;
      break;
    }
    case 1:
      if (Lerp(&open_, 0, 1.0f, 10, mode_time_)) {
        ++step_;
      }
      break;
    case 2:
      if (Lerp(&back_alpha_, 0, 1.0f, 10, mode_time_)) {
        ++step_;
      }
      break;
    case 3:
      if (Lerp(&text_alpha_, 0, 1.0f, 10, mode_time_)) {
        step_ = 10;
      }
      break;
    case 10:
      last_dirs_ = 0;
      dirs_time_ = 0;
      flags_ |= kFlagNameCursor;
      step_ = 11;
    case 11:
      if (EnterName()) {
        flags_ &= ~kFlagNameCursor;
        strcpy(g_global.entry.name, name_);
        g_score_table.Insert(&g_global.entry, -1);
        g_score_table.Store();
        step_ = 20;
      }
      break;
    case 20:
      Lerp(&text_alpha_, 1.0f, 0, 10, mode_time_);
      if (Lerp(&back_alpha_, 1.0f, 0, 10, mode_time_)) {
        ++step_;
      }
      break;
    case 21:
      if (Lerp(&open_, 1.0f, 0, 10, mode_time_)) {
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

// 0x41f830
void GameHud::DrawNameEntry() {
  HL3D* d3d = g_application.d3d;
  Transform transform;
  transform.m[2][2] = 1;
  transform.m[1][1] = 1;
  transform.m[0][0] = 1;
  transform.m[1][2] = 0;
  transform.m[1][0] = 0;
  transform.m[0][2] = 0;
  transform.m[0][1] = 0;
  transform.m[2][0] = 320;
  transform.m[2][1] = 240;
  SetTexture(d3d, NULL, 0);
  g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, &transform, kBackdrop,
                                static_cast<int>(back_alpha_ * 160) << 24, 4);

  // The bars, scrolling apart as the window opens.
  transform.m[2][2] = 1;
  transform.m[1][1] = 1;
  transform.m[0][0] = 1;
  transform.m[1][2] = 0;
  transform.m[1][0] = 0;
  transform.m[0][2] = 0;
  transform.m[0][1] = 0;
  transform.m[2][0] = 0;
  transform.m[2][1] = 0;
  ScaleTransform(&transform, 1.0f, open_);
  transform.m[2][0] = -static_cast<float>(
      static_cast<unsigned int>(g_application.frame_count & 0x3f) * 2);
  transform.m[2][1] -= 52;
  Transform turned;
  turned.m[0][0] = transform.m[0][0];
  turned.m[0][1] = transform.m[0][1];
  turned.m[0][2] = transform.m[0][2];
  turned.m[1][0] = transform.m[1][0];
  turned.m[1][1] = transform.m[1][1];
  turned.m[1][2] = transform.m[1][2];
  turned.m[2][0] = transform.m[2][0];
  turned.m[2][1] = transform.m[2][1];
  turned.m[2][2] = transform.m[2][2];
  SetTexture(g_application.d3d, &g_fonts.textures[2], 0);
  g_application.d3d->SetFilter(open_ != 1.0f, 0);
  transform.m[2][0] += 320;
  transform.m[2][1] += 240;
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &transform, kBarPoints,
                                kBarTexels, 0xc800ff80, 4);
  RotateTransform(&turned, 0x200);
  turned.m[2][0] += 320;
  turned.m[2][1] += 240;
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &turned, kBarPoints,
                                kBarTexels, 0xc800ff80, 4);
  g_application.d3d->SetPointFilter(0);

  if (flags_ & kFlagNameCursor) {
    g_application.d3d->ClearTexture();
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = cursor_ * 16.0f + 296;
    transform.m[2][1] = 244;
    g_application.d3d->ClearTexture();
    g_application.d3d->DrawPoints(
        D3DPT_LINELIST, &transform, kNameCursor,
        static_cast<int>((1 - (mode_time_ & 0x3f) * (1 / 63.0f)) * 200) << 24 |
            0xffc800,
        2);
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = char_index_ * 8.0f + 116;
    transform.m[2][1] = 268;
    g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, &transform, kCharCursor,
                                  0x6400ff80, 4);
  }

  D3DCOLOR color = static_cast<int>(text_alpha_ * 200) << 24 | 0xffc800;
  D3DCOLOR white = static_cast<int>(text_alpha_ * 128) << 24 | 0xffffff;
  mode_texts_[0]->Print(184, 204, color, -1);
  mode_texts_[1]->Print(228, 228, white, -1);
  mode_texts_[2]->Print(296, 244, color, -1);
  mode_texts_[3]->Print(116, 268, white, -1);
}
