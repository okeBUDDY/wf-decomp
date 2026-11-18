// cGlobal: start-up, the data it loads, and shutdown. Sprite::SetColor()
// is defined here too, so these functions inline it.
#include "cglobal.h"

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "enm_param.h"
#include "file.h"
#include "fn_game.h"
#include "fn_option.h"
#include "fonts.h"
#include "game.h"
#include "game_data.h"
#include "hl_3d.h"
#include "score_table.h"
#include "snd_sys.h"
#include "sounds.h"
#include "sprite.h"

// Implicit ctor 0x401bf0, dtor 0x401e00.
PatFiles g_pats;  // 0x467d70
// Implicit ctor 0x401e90, dtor 0x401f80.
// TODO: nonmatching (the implicit dtor 0x401f80: the original interleaves
// the vector dtor's argument pushes with the vptr stores).
Fonts g_fonts;  // 0x467ec0

// 0x4010a0
cGlobal::cGlobal() {
  music_volume = effect_volume = 1.0f;
  name_ = "cGlobal";
  g_boss = g_player = NULL;
}

// 0x4010d0
void cGlobal::Finalize() {
  g_actor_system.DestroyAll();
  g_pats.sys[0].Release();
  g_pats.enemy[0].Release();
  g_pats.add[0].Release();
  for (int i = 0; i < 3; ++i) {
    g_pats.etc[i].Release();
  }
  g_fonts.text.Release();
  g_fonts.small_text.Release();
  g_fonts.kana.Release();
  g_fonts.title.Release();
  g_fonts.large_digit.Release();
  g_fonts.digit.Release();
  g_fonts.textures[0].Release();
  g_fonts.textures[1].Release();
  g_fonts.textures[2].Release();
  for (int i = 0; i < kSounds; ++i) {
    g_sounds[i].Release();
  }
  g_bgm.Release();
  ClosePack();
}

// 0x4011a0
void cGlobal::ClearStage() {
  g_actor_system.DestroyAll();
  screen_fade->SetColor(0, 0, 0, 0);
}

// 0x4011f0
void cGlobal::ResetSound() {
  g_application.sound->Stop(NULL, -1);
  g_application.sound->MuteGroup(0, -1);
  g_application.sound->SetGroupVolume(music_volume, 0);
  g_application.sound->SetGroupVolume(effect_volume, 1);
}

// 0x401240
void cGlobal::ResetGame() {
  g_boss = g_player = NULL;
  stage = 0;
  entry.score = 0;
  entry.deaths = 0;
  if (g_fn_game.mode() == 0) {
    g_score_table.ClearRank();
  }
  g_enm_param.Reset();
}

// 0x401280
int cGlobal::LoadFonts() {
  if (!g_fonts.textures[0].Load("font0.png", 0x44)) {
    return 0;
  }
  if (!g_fonts.textures[1].Load("font1.png", 0x44)) {
    return 0;
  }
  if (!g_fonts.textures[2].Load("font2.png", 0x44)) {
    return 0;
  }
  if (!g_fonts.text.Initialize(&g_application, &g_fonts.textures[1], 32, " ",
                               64, 0, 192, 16, 16, 16, 0, NULL)) {
    return 0;
  }
  if (!g_fonts.small_text.Initialize(&g_application, &g_fonts.textures[0], 32,
                                     " ", 64, 128, 64, 8, 8, 16, 0, NULL)) {
    return 0;
  }
  if (!g_fonts.kana.Initialize(&g_application, &g_fonts.textures[1], 8, "ァ",
                               96, 0, 0, 16, 32, 16, 0, NULL)) {
    return 0;
  }
  if (!g_fonts.title.Initialize(&g_application, &g_fonts.textures[0], 8, "A",
                                28, 0, 0, 20, 32, 12, 9, g_title_font_widths)) {
    return 0;
  }
  if (!g_fonts.large_digit.Initialize(&g_application, &g_fonts.textures[0], 1,
                                      "0", 10, 0, 96, 48, 80, 5, 0, NULL)) {
    return 0;
  }
  if (!g_fonts.digit.Initialize(&g_application, &g_fonts.textures[2], 1, "0",
                                10, 0, 48, 24, 40, 5, 0, NULL)) {
    return 0;
  }
  g_fonts.kana.Map("　", 87);
  g_fonts.kana.Map("ー", 88);
  g_fonts.kana.Map("～", 89);
  g_fonts.kana.Map("★", 90);
  g_fonts.kana.Map("☆", 91);
  g_fonts.title.Map("-", 26);
  return 1;
}

// 0x401470
int cGlobal::LoadPatterns() {
  if (!g_pats.sys[0].Load("sys0", 0)) {
    return 0;
  }
  if (!g_pats.enemy[0].Load("enm0", 0)) {
    return 0;
  }
  if (!g_pats.add[0].Load("add0", 0)) {
    return 0;
  }
  if (!g_pats.etc[0].Load("etc0", 0)) {
    return 0;
  }
  if (!g_pats.etc[1].Load("etc1", 0)) {
    return 0;
  }
  if (!g_pats.etc[2].Load("etc2", 0)) {
    return 0;
  }
  return 1;
}

// 0x401500
void ActorSystem::SetMargin(int x, int y) {
  left_ = static_cast<float>(-x);
  right_ = static_cast<float>(app_->width + x);
  top_ = static_cast<float>(-y);
  bottom_ = static_cast<float>(app_->height + y);
}

// 0x401550
int cGlobal::LoadSounds() {
  if (!g_sounds[kSndBomb1].Load("bomb_1.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBomb2].Load("bomb_2.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBomb3].Load("bomb_3.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBomb4].Load("bomb_4.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBombPre].Load("bomb_pre.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBombAll].Load("bomb_all.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBombMe].Load("bomb_me.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndDamage].Load("damage.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndNoDamage].Load("nodamage.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndWarning].Load("warning.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndSwitch].Load("switch.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndWeapon0].Load("wep0.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndWeapon1].Load("wep1.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireMe].Load("fire_me.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireBl].Load("fire_bl.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireCn].Load("fire_cn.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireLs1].Load("fire_ls1.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireLs2].Load("fire_ls2.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireLs3].Load("fire_ls3.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndFireMs].Load("fire_ms.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBarrier].Load("barrier.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBoost1].Load("boost1.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndBoost2].Load("boost2.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndTrans1].Load("trans1.ogg", 0, 0)) {
    return 0;
  }
  if (!g_sounds[kSndTrans2].Load("trans2.ogg", 0, 0)) {
    return 0;
  }
  g_actor_system.SetDamageSound(Actor::kDamageArmor, &g_sounds[kSndDamage],
                                g_application.sound->channel(kSndDamage));
  g_actor_system.SetDamageSound(Actor::kDamageGuard, &g_sounds[kSndNoDamage],
                                g_application.sound->channel(kSndNoDamage));
  if (!g_bgm.Load("bgm.seq", 0x1000000, 1) &&
      !g_bgm.Load("bgm.ogg", 0x1000000, 1)) {
    g_bgm.Load("bgm.wav", 0x1000000, 1);
  }
  Object::ClearError();
  return 1;
}

// 0x401880
// TODO: nonmatching: the original keeps the two Error() returns apart and
// sends the failed loads to the second one; this merges both Error() calls.
int cGlobal::Initialize(Application* app) {
  if (!OpenPack("data.bin")) {
    Error("data.bin がオープンできません");
    return 0;
  }
  flags = 0;
  if (!LoadFonts() || !LoadPatterns() || !RegisterActorControls() ||
      !LoadSounds()) {
    return 0;
  }
  g_application.d3d->SetCullMode(D3DCULL_NONE);
  g_application.d3d->SetTextureAddress(D3DTADDRESS_WRAP, 0);
  g_actor_system.SetMargin(100, 32);
  g_score_table.Init("wf.sco");
  g_fn_option.LoadSettings();
  screen_fade = new Sprite;
  if (screen_fade == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  screen_fade->app_ = &g_application;
  g_application.overlays.AddBack(screen_fade, 1);
  InitRecords();
  return 1;
}
