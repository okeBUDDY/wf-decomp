// fnTitle: the title screen.
#include "fn_title.h"

#include <string.h>

#include "actor_system.h"
#include "application.h"
#include "background.h"
#include "cglobal.h"
#include "fonts.h"
#include "game_data.h"
#include "game_hud.h"
#include "hls_pat.h"
#include "hls_txt.h"
#include "score_table.h"
#include "sprite.h"

// 0x41df30
FnTitle::FnTitle() {
  name_ = "fnTitle";
  texts_[0] = texts_[1] = texts_[2] = NULL;
  unknown_50_ = 0;
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
  time_ = 100;
  unknown_56_ = 0;
  unknown_50_ = 0;
  unknown_48_ = 0;
  scores_shown_ = 0;
  menu_x_ = -240;
  cursor_alpha_ = 0;
  unknown_46_ = 0;
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
