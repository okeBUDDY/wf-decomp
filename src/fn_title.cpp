// fnTitle: the title screen.
#include "fn_title.h"

#include <string.h>

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "background.h"
#include "cglobal.h"
#include "fn_game.h"
#include "fn_option.h"
#include "fonts.h"
#include "game_data.h"
#include "game_hud.h"
#include "hl_input.h"
#include "hls_pat.h"
#include "hls_txt.h"
#include "score_table.h"
#include "snd_sys.h"
#include "sounds.h"
#include "sprite.h"

// 0x41df30
FnTitle::FnTitle() {
  name_ = "fnTitle";
  texts_[0] = texts_[1] = texts_[2] = NULL;
  menu_ = 0;
}

// 0x41df70
void FnTitle::Finalize() {
  g_actor_system.DestroyAll();
  if (texts_[0] != NULL) {
    texts_[0]->Release();
    texts_[0] = NULL;
  }
  if (texts_[1] != NULL) {
    texts_[1]->Release();
    texts_[1] = NULL;
  }
  if (texts_[2] != NULL) {
    texts_[2]->Release();
    texts_[2] = NULL;
  }
  g_application.tasks.Delete(g_title_background);
  g_title_background = NULL;
}

char kTitle[] = "WARNING   FOREVER";  // 0x45dbbc
const char* kMenuItems[] = {"GAME_START", "  REPLAY", "  OPTION",
                            "   EXIT"};  // 0x45dbd0
// The arrows above and below the menu item.
D3DVECTOR kMenuCursor[8] = {{0, -24, 0}, {-8, -16, 0}, {0, -24, 0},
                            {8, -16, 0}, {0, 24, 0},   {-8, 16, 0},
                            {0, 24, 0},  {8, 16, 0}};  // 0x45dbe8

// 0x41e050
int FnTitle::Initialize() {
  g_global.ClearStage();
  BackgroundGrid* background = new BackgroundGrid;
  g_title_background = background;
  if (background == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  background->app_ = &g_application;
  background->color_ = 0xff004030;
  background->speed_ = 8.0f;
  g_application.tasks.AddBack(background, 2);
  texts_[0] = g_text_font.Create(0x20, NULL);
  texts_[1] = g_text_font.Create(0x40, NULL);
  texts_[2] = g_small_font.Create(0x200, NULL);
  scores_length_ = g_score_table.Format(texts_[2]->text());
  title_length_ = strlen(kTitle);

  ActorDesc desc;
  memset(&desc, 0, sizeof(desc));
  desc.flags = 0;
  desc.team = 0;
  desc.hit_points = 1;
  desc.sprite = NULL;
  desc.parent = NULL;
  desc.y = 240;
  desc.layer = 4;
  desc.color = 0xffffffff;
  desc.x = 800;
  HLSObject* sprite = g_etc_pats[0].Get(1);
  desc.sprite = sprite;
  sweeps_[0] = g_actor_system.Spawn(&desc);
  desc.x = -160;
  sprite = g_etc_pats[0].Get(2);
  desc.sprite = sprite;
  sweeps_[1] = g_actor_system.Spawn(&desc);
  desc.color = 0xffffd000;
  desc.x = 184;
  desc.y = 304;
  desc.sprite = texts_[0];
  subtitle_ = g_actor_system.Spawn(&desc);

  desc.layer = 4;
  desc.flags = 0;
  desc.team = 0;
  desc.hit_points = 1;
  desc.sprite = NULL;
  desc.parent = NULL;
  desc.x = 320;
  desc.y = 220;
  desc.color = 0x00ffffff;
  sprite = g_etc_pats[2].Get(1);
  desc.sprite = sprite;
  logo_[0] = g_actor_system.Spawn(&desc);
  sprite = g_etc_pats[2].Get(2);
  desc.sprite = sprite;
  logo_[1] = g_actor_system.Spawn(&desc);

  hex_scale_ = 0;
  time_ = kStepLogo;
  step_time_ = 0;
  menu_ = 0;
  title_shown_ = 0;
  scores_shown_ = 0;
  menu_x_ = -240;
  cursor_alpha_ = 0;
  logo_alpha_ = 0;
  flags_ = 0;
  strcpy(texts_[1]->text(), kMenuItems[0]);
  g_global.screen_fade->FadeIn(120);
  return 1;
}

// 0x41eae0
void FnTitle::Draw() {
  if (hex_scale_ > 0) {
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
    ScaleTransform(&transform, hex_scale_, hex_scale_);
    RotateTransform(&transform, g_application.frame_count * 3);
    transform.m[2][0] += 300;
    transform.m[2][1] += 320;
    g_application.d3d->DrawPoints(D3DPT_LINESTRIP, &transform, g_hexagon,
                                  0x8000ff80, 7);
  }
  if (flags_ & kFlagMenuCursor) {
    Transform transform;
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][0] = 128;
    transform.m[2][1] = 408;
    g_application.d3d->DrawPoints(D3DPT_LINELIST, &transform, kMenuCursor,
                                  cursor_alpha_ << 24 | 0xffdd00, 8);
  }
  texts_[1]->Print(menu_x_ + 48, 400, 0x80ffffff, -1);
  if (time_ >= 301) {
    int alpha = (-1 - g_application.frame_count * 4) & 0xff;
    D3DCOLOR palette[2];
    palette[0] = 0x8000ff80;
    palette[1] = ((((alpha >> 1) - 0x80) << 8 | alpha) << 16) | 0xff80;
    texts_[2]->Print(32, 48, palette, scores_shown_);
  }
  g_actor_system.DrawAll();
}

// 0x41e300
void FnTitle::Move() {
  ++step_time_;
  unsigned short time = time_;
  switch (time_) {
    case kStepLogo:
      if (g_global.screen_fade->fade_frames() == 0) {
        logo_alpha_ = 0;
        time_ = time + 1;
      }
      break;
    case kStepLogo + 1:
      logo_alpha_ += 4;
      if (logo_alpha_ >= 255) {
        logo_alpha_ = 255;
        time_ = time + 1;
      }
      logo_[1]->color = logo_alpha_ << 24 | 0xffffff;
      logo_[0]->color = logo_[1]->color;
      break;
    case kStepLogo + 2:
      if (step_time_ == 120) {
        time_ = time + 1;
      }
      break;
    case kStepLogo + 3:
      logo_alpha_ -= 4;
      if (logo_alpha_ <= 0) {
        logo_alpha_ = 0;
        time_ = time + 1;
      }
      logo_[1]->color = logo_alpha_ << 24 | 0xffffff;
      logo_[0]->color = logo_[1]->color;
      break;
    case kStepLogo + 4:
      if (step_time_ == 60) {
        time_ = kStepIntro;
      }
      break;
    case kStepIntro:
      if (g_global.screen_fade->fade_frames() == 0) {
        sweep_offset_ = 480;
        time_ = time + 1;
      }
      break;
    case kStepIntro + 1:
      if ((sweep_offset_ *= 0.8f) < 1.0f) {
        sweep_offset_ = 0;
        time_ = time + 1;
      }
      sweeps_[0]->SetX(sweep_offset_ + 320);
      sweeps_[1]->SetX(320 - sweep_offset_);
      break;
    case kStepIntro + 2:
      if (step_time_ == 13) {
        time_ = time + 1;
      }
      sweeps_[0]->SetY(sweeps_[0]->y() - 4);
      sweeps_[1]->SetY(sweeps_[1]->y() + 4);
      break;
    case kStepIntro + 3:
      if (step_time_ % 3 == 0) {
        ++title_shown_;
        strncpy(texts_[0]->text(), kTitle, title_shown_);
        texts_[0]->text()[title_shown_] = '\0';
        if (title_shown_ >= title_length_) {
          ++time_;
        }
      }
      break;
    case kStepIntro + 4:
      if (step_time_ == 8) {
        time_ = time + 1;
      }
      sweeps_[0]->SetY(sweeps_[0]->y() + 8);
      sweeps_[1]->SetY(sweeps_[1]->y() + 8);
      subtitle_->SetY(subtitle_->y() + 8);
      break;
    case kStepIntro + 5:
      if (step_time_ == 14) {
        time_ = time + 1;
      }
      sweeps_[0]->SetX(sweeps_[0]->x() + 11);
      sweeps_[1]->SetX(sweeps_[1]->x() + 11);
      subtitle_->SetX(subtitle_->x() + 11);
      break;
    case kStepIntro + 6:
      if ((hex_scale_ += 4) >= 65) {
        hex_scale_ = 65;
        time_ = kStepTitle;
      }
      break;
    case kStepTitle:
      sweeps_[0]->SetPosition(474, 252);
      sweeps_[1]->SetPosition(474, 356);
      subtitle_->SetPosition(338, 368);
      logo_[1]->color = 0xffffff;
      logo_[0]->color = 0xffffff;
      strcpy(texts_[0]->text(), kTitle);
      hex_scale_ = 65;
      g_global.screen_fade->SetColor(0, 0, 0, 0);
      flags_ |= kFlagTitleShown;
      ++time_;
      break;
    case kStepMenu:
      if (menu_x_ != 0) {
        menu_x_ = static_cast<short>(
            (1.0f - g_sin_table[(step_time_ << 8) / 15 & 0x3ff]) * -240);
      } else {
        flags_ |= kFlagMenuCursor;
        time_ = time + 1;
      }
      // Falls through.
    case kStepMenu + 1: {
      unsigned short pressed = g_application.input->pad(0)->pressed();
      if (pressed &
          (FnGame::kKeyFire | FnGame::kKeySlow | FnGame::kKeySwitch)) {
        switch (menu_) {
          case kMenuStart:
            g_fn_game.SetMode(0, NULL);
            g_application.user_func->Finish(&g_fn_game);
            break;
          case kMenuReplay:
            g_global.ClearReplay();
            if (g_global.NextReplay(g_text_buffer)) {
              g_fn_game.SetMode(1, g_text_buffer);
              time_ = kStepSelected;
            }
            break;
          default:
            time_ = kStepSelected;
            break;
        }
        g_application.sound->channel(kSndSwitch)
            ->Assign(&g_sounds[kSndSwitch], 0, 1.0f);
        break;
      }
      if (pressed & (HLInpDev::kUp | HLInpDev::kDown)) {
        if (pressed & HLInpDev::kDown) {
          if (++menu_ >= kMenuItemCount) {
            menu_ = 0;
          }
        } else {
          if (--menu_ < 0) {
            menu_ = kMenuItemCount - 1;
          }
        }
        texts_[1]->SetText(kMenuItems[menu_]);
        cursor_alpha_ = 255;
        g_application.sound->channel(kSndFireMe)
            ->Assign(&g_sounds[kSndFireMe], 0, 1.0f);
        if (step_time_ > 480) {
          step_time_ = 480;
        }
      }
      if (step_time_ >= 960) {
        time_ = kStepDemo;
      }
      break;
    }
    case kStepDemo:
    case kStepSelected:
      g_global.screen_fade->FadeTo(60, 0, 0, 0, 255);
      ++time_;
      break;
    case kStepDemo + 1:
      if (g_global.screen_fade->fade_frames() == 0) {
        if (g_global.NextReplay(g_text_buffer)) {
          g_fn_game.SetMode(2, g_text_buffer);
          g_application.user_func->Finish(&g_fn_game);
        } else {
          g_application.user_func->Finish(&g_fn_title);
        }
      }
      break;
    case kStepSelected + 1:
      if (g_global.screen_fade->fade_frames() == 0) {
        switch (menu_) {
          case kMenuReplay:
            g_application.user_func->Finish(&g_fn_game);
            break;
          case kMenuOption:
            g_application.user_func->Finish(&g_fn_option);
            break;
          case kMenuExit:
            g_application.Quit();
            break;
        }
      }
      break;
  }
  if (!(flags_ & kFlagTitleShown) &&
      (g_application.input->pad(0)->pressed() &
       (FnGame::kKeyFire | FnGame::kKeySlow | FnGame::kKeySwitch))) {
    menu_x_ = 0;
    time_ = kStepTitle;
  }
  if (flags_ & kFlagMenuCursor) {
    cursor_alpha_ -= 4;
    if (cursor_alpha_ <= 0) {
      cursor_alpha_ = 255;
    }
    if (scores_shown_ < scores_length_ && (g_application.frame_count & 1)) {
      ++scores_shown_;
    }
  }
  if (time != time_) {
    step_time_ = 0;
  }
  g_actor_system.MoveAll();
}
