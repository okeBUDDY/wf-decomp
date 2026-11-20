// fnGame: the stage that runs a game, and fnOption: the option screen. The
// original compiles both in one object, so FnOption's functions inline
// helpers defined with FnGame (Sprite::FadeIn()).
#include "fn_game.h"

#include <stdio.h>
#include <string.h>

#include "actor.h"
#include "actor_control.h"
#include "actor_system.h"
#include "application.h"
#include "background.h"
#include "cglobal.h"
#include "enm_param.h"
#include "fn_option.h"
#include "fn_title.h"
#include "fonts.h"
#include "game.h"
#include "game_hud.h"
#include "hl_3d.h"
#include "hl_input.h"
#include "hls_txt.h"
#include "replay.h"
#include "snd_file_seq.h"
#include "snd_sys.h"
#include "sounds.h"
#include "sprite.h"

// 0x41be00
FnGame::FnGame() {
  name_ = "fnGame";
  background_ = NULL;
  hud_ = NULL;
  mode_ = 0;
}

// 0x41be30
void FnGame::SetMode(int mode, const char* path) {
  mode_ = mode;
  switch (mode) {
    case 0:
      g_replay.Open(Replay::kRecord, "rec\\temp.rec");
      break;
    case 1:
    case 2:
      g_replay.Open(Replay::kPlay, path);
      break;
  }
}

// 0x41be80
void FnGame::SetState(short state) {
  state_ = state;
  state_time_ = 0;
}

// 0x41bea0
void FnGame::Draw() { g_actor_system.DrawAll(); }

// 0x41beb0
void Sprite::FadeIn(short frames) {
  SetColor(0, 0, 0, 0xff);
  FadeTo(frames, 0, 0, 0, 0);
}

// 0x41bf00
void Sprite::FadeOut(short frames) {
  SetColor(0, 0, 0, 0);
  FadeTo(frames, 0, 0, 0, 0xff);
}

// 0x41c140
void FnGame::Exit(int fade, int wait) {
  if (exit_frames_ == 0) {
    exit_frames_ = fade + wait;
    exit_time_ = 0;
    g_global.screen_fade->FadeOut(fade);
  }
}

// 0x41c170
void FnGame::SetSeqFlags() {
  ClearSeqFlags();
  g_seq_flags[0] = 1;
  g_seq_flags[4] = static_cast<unsigned char>(bgm_mode_);
}

// 0x41c190
void FnGame::Move() {
  if (flags_ & kFlagPlayback) {
    unsigned short pressed = g_application.input->pad(0)->pressed();
    if (pressed & (kKeyFire | kKeySlow | kKeySwitch)) {
      if (pressed & kKeySlow) {
        flags_ &= ~kFlagReplayList;
      }
      Exit(30, 0);
    }
  }
  ++state_time_;
  int battle = 0;
  switch (state_) {
    case kStateBattle:
      ActorSystem::flags_ |= 1;
      g_game.flags_ |= Game::kRunning;
      g_boss = g_enm_param.StartBattle(g_player);
      ++state_;
      // Falls through.
    case kStateBattle + 1:
      g_enm_param.RunBattle(g_player, g_boss);
      battle = 1;
      break;
    case kStateStage:
      ++g_global.stage;
      if (flags_ & kFlagMusic) {
        RateGame();
        switch (bgm_mode_) {
          case 1:
            if (g_global.stage > 1) {
              g_application.sound->channel(kChannelBgm)
                  ->Fade(0.0f, 1.0f, 3.0f, 0);
            }
            break;
          case 2:
            g_application.sound->channel(kChannelBgm)
                ->Assign(&g_bgm, Channel::kLoop, 1.0f);
            break;
        }
      }
      hud_->RequestMode(GameHud::kModeGameOver);
      g_replay.Start(g_player);
      ++state_;
      // Falls through.
    case kStateStage + 1:
      if (!hud_->busy()) {
        SetState(kStateBattle);
      }
      break;
    case kStateCleared:
      g_game.flags_ &= ~Game::kRunning;
      g_game.OnBossDestroyed();
      g_replay.Close();
      if (flags_ & kFlagMusic) {
        switch (bgm_mode_) {
          case 1:
            g_application.sound->channel(kChannelBgm)
                ->Fade(1.0f, 0.0f, 5.0f, 0);
            break;
          case 2:
            g_application.sound->channel(kChannelBgm)
                ->Fade(1.0f, 0.0f, 5.0f, 1);
            break;
        }
      }
      if (!(flags_ & kFlagPlayback)) {
        g_global.SaveReplay(0);
        hud_->RequestMode(GameHud::kModeResult);
      }
      ActorSystem::flags_ &= ~1;
      ++state_;
      // Falls through.
    case kStateCleared + 1:
      if (!hud_->busy() && g_boss == NULL) {
        ++state_;
      }
      break;
    case kStateCleared + 2:
      if (flags_ & kFlagPlayback) {
        Exit(60, 60);
      } else {
        g_enm_param.Plan(g_global.entry.score - stage_score_);
        stage_score_ = g_global.entry.score;
        SetState(kStateStage);
      }
      break;
    case kStateGameOver:
      g_game.flags_ &= ~Game::kRunning;
      g_replay.Close();
      if (g_player != NULL) {
        g_player->Destroy(1, 1);
      }
      if (!(flags_ & kFlagPlayback)) {
        g_global.SaveReplay(1);
        g_global.entry.stage = static_cast<short>(g_global.stage);
        g_global.entry.time = g_game.frames_;
        hud_->RequestMode(GameHud::kModeNameEntry);
      }
      ++state_;
      // Falls through.
    case kStateGameOver + 1:
      if (!hud_->busy() && exit_frames_ == 0) {
        Exit(60, 60);
      }
      g_enm_param.RunBattle(g_player, g_boss);
      break;
    case kStateStart:
      if (!g_replay.LoadHeader()) {
        g_application.user_func->Finish(&g_fn_title);
        break;
      }
      ActorSystem::flags_ &= ~1;
      g_game.SpawnPlayer();
      if (g_player == NULL || !g_replay.LoadPlayer(g_player)) {
        g_application.user_func->Finish(&g_fn_title);
        break;
      }
      if (flags_ & kFlagMusic) {
        SetSeqFlags();
        if (bgm_mode_ != 2) {
          g_application.sound->channel(kChannelBgm)
              ->Assign(&g_bgm, Channel::kLoop, 1.0f);
        }
      }
      g_global.screen_fade->FadeIn(60);
      ++state_;
      // Falls through.
    case kStateStart + 1:
      if (state_time_ >= 120) {
        SetState(kStateStage);
      }
      break;
  }
  g_game.Tick();
  if (battle && (g_game.flags_ & Game::kGameOver)) {
    SetState(kStateGameOver);
  }
  if (exit_frames_ != 0) {
    if (++exit_time_ >= exit_frames_) {
      if ((flags_ & kFlagReplayList) && g_global.NextReplay(g_text_buffer)) {
        mode_ = 1;
        g_replay.Open(Replay::kPlay, g_text_buffer);
        g_application.user_func->Finish(&g_fn_game);
      } else {
        g_application.user_func->Finish(&g_fn_title);
      }
    }
    float volume = 1.0f - static_cast<float>(exit_time_) / exit_frames_;
    g_application.sound->SetGroupVolume(g_global.effect_volume * volume, 0);
    g_application.sound->SetGroupVolume(g_global.music_volume * volume, 1);
  }
  g_replay.GetInput(&keys_, &pressed_);
  if (flags_ & kFlagMusic) {
    int seconds = g_game.timer_ / 60;
    if (seconds > 127) {
      seconds = 127;
    }
    g_seq_flags[3] = static_cast<unsigned char>(seconds);
  }
  g_actor_system.MoveAll();
}

// 0x41bf50
// TaskList::Add is called directly here, not through AddBack(): the inline
// wrapper's parameter copy shifts VC's register rotation, and the input
// pointer at the end then lands in eax instead of ecx.
int FnGame::Initialize() {
  g_global.ClearStage();
  g_global.ResetGame();
  g_game.Start(&g_global.rules);
  Control01::ResetWeapon();
  stage_score_ = 0;
  exit_time_ = 0;
  exit_frames_ = 0;
  flags_ = 0;
  BackgroundGrid* background = new BackgroundGrid;
  background_ = background;
  if (background == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  background->app_ = &g_application;
  background->color_ = 0xff004030;
  background->speed_ = 8.0f;
  g_application.tasks.Add(background_, 2, TASK_APPEND);
  GameHud* hud = new GameHud;
  hud_ = hud;
  if (hud == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  g_application.overlays.Add(hud, 2, NULL);
  SetState(kStateStart);
  int pause = g_global.flags & cGlobal::kFlagPause;
  switch (mode_) {
    case 0:
      if (g_bgm.loaded()) {
        flags_ |= kFlagMusic;
      }
      break;
    case 1:
      flags_ |= kFlagPlayback | kFlagReplayList;
      break;
    case 2:
      g_application.sound->MuteGroup(1, -1);
      flags_ |= kFlagPlayback;
      return 1;
  }
  if (pause) {
    g_application.input->set_time_stop_mask(0x8000);
  }
  return 1;
}

// 0x41c0e0
void FnGame::Finalize() {
  g_replay.Close();
  g_application.input->set_time_stop_mask(0);
  g_actor_system.DestroyAll();
  g_global.ResetSound();
  g_application.tasks.Delete(background_);
  background_ = NULL;
  g_application.overlays.Delete(hud_);
  hud_ = NULL;
}

// The kinds of menu items.
enum OptionKind {
  kKindChoice = 0,     // one of the choices in the text
  kKindFormatted = 1,  // a value printed into the text
  kKindAction = 2,     // starts something when a trigger is pressed
  kKindButton = 3,     // takes the next trigger pressed
};

struct OptionItem {
  short kind;          // OptionKind
  short count;         // of the values
  short column;        // of the value in the text: characters, or pixels
  short width;         // of the cursor under the value, pixels
  unsigned int param;  // kKindChoice: pixels per value; kKindButton: trigger
  const char* text;
};

// The menu (the formatted widths are set as the values change).
static OptionItem kOptionItems[FnOption::kItems] = {
    {kKindChoice, 2, 0, 64, 72, "BOSS NAME       : JAPANESE ENGLISH "},
    {kKindChoice, 10, 32, 8, 8, "BGM VOLUME      : MIN ---------- MAX"},
    {kKindChoice, 10, 32, 8, 8, "SE VOLUME       : MIN ---------- MAX"},
    {kKindChoice, 3, 0, 32, 48, "BGM MODE        : LOOP  FADE  REW "},
    {kKindChoice, 2, 0, 32, 48, "SCORE RESET     : NO    YES "},
    {kKindChoice, 2, 0, 32, 48, "STAGE REPLAY    : NO    YES "},
    {kKindChoice, 2, 0, 32, 48, "SETUP DIALOGBOX : HIDE  SHOW"},
    {kKindFormatted, 5, 16, 8, 0, "GAME MODE       : %s%s%s"},
    {kKindFormatted, 10, 16, 8, 0, "START TIME      : %s%s%s"},
    {kKindFormatted, 2, 16, 8, 0, "TIMER MODE      : %s%s%s"},
    {kKindFormatted, 10, 16, 8, 0, "NUMBER OF SHIPS : %s%s%s"},
    {kKindFormatted, 3, 16, 8, 0, "SHIP EXTEND     : %s%s%s"},
    {kKindButton, 0, 25, 0, 0, "SHOT            : BUTTON   "},
    {kKindButton, 0, 25, 0, 1, "SLOW MOVE       : BUTTON   "},
    {kKindButton, 0, 25, 0, 3, "WEAPON SELECT   : BUTTON   "},
    {kKindChoice, 2, 0, 56, 72, "GAME PAUSE      : DISABLE  ENABLE"},
    {kKindButton, 0, 25, 0, 7, "PAUSE BUTTON    : BUTTON   "},
    {kKindAction, 0, 0, 0, 0, "EXIT"},
};  // 0x45da50

// The bar under the menu title, and its texels in font2.png.
static D3DVECTOR kTitleBar[4] = {
    {0, -8, 0}, {768, -8, 0}, {0, 8, 0}, {768, 8, 0}};  // 0x45db70
static POINTS kTitleBarTexels[4] = {
    {0, 16}, {768, 16}, {0, 32}, {768, 32}};  // 0x45dba0

int FnOption::presets_[][kRules] = {
    {5, 0, 0, 0},  // NORMAL
    {0, 0, 3, 2},  // 3 SHIPS
    {0, 0, 1, 0},  // SUDDEN DEATH
    {7, 1, 0, 0},  // 5 MINUTES ATTACK
    {5, 0, 0, 0},  // CUSTOM
};  // 0x45d9b0

// Seconds of each START TIME, 0: none.
static int kStartTimes[] = {0,   10,  30,  60,  120,
                            180, 240, 300, 600, 999};  // 0x45da00
static const char* kModeNames[] = {"NORMAL", "3 SHIPS", "SUDDEN DEATH",
                                   "5 MINUTES ATTACK", "CUSTOM"};  // 0x45da28
static const char* kTimerModeNames[] = {"NORMAL",
                                        "COUNT DOWN ONLY"};  // 0x45da3c
static const char* kExtendNames[] = {"NO EXTEND", "BY STAGE CLEAR",
                                     "BY 100 PARTS"};  // 0x45da44

// The volume of each step of the volume items.
static const float kVolumeStep = 1.0f / 9;

inline void HLSTxt::BindTexture() {
  SetTexture(system_->app_->d3d, texture_, 0);
}

inline void HLSTxt::PrintInline(int x, int y, D3DCOLOR color, int count) {
  BindTexture();
  system_->Print(static_cast<float>(x), static_cast<float>(y), text_, NULL,
                 color, count);
}

// The trigger of the button `item`.
static inline int Trigger(int item) {
  return g_application.input->pad(0)->trigger(kOptionItems[item].param);
}

static __forceinline void PlayEffect(int id) {
  g_application.sound->channel(id)->Assign(&g_sounds[id], 0, 1.0f);
}

// 0x41c6a0
void cGlobal::SetEffectVolume(float volume) {
  effect_volume = volume;
  g_application.sound->SetGroupVolume(volume, 0);
}

// 0x41c6c0
void cGlobal::SetMusicVolume(float volume) {
  music_volume = volume;
  g_application.sound->SetGroupVolume(volume, 1);
}

// 0x41c6e0
FnOption::FnOption() {
  name_ = "fnOption";
  for (int i = 0; i < kItems; ++i) {
    items_[i] = NULL;
    enabled_[i] = 1;
  }
  background_ = NULL;
}

// 0x41c730
void FnOption::Apply() {
  g_global.music_volume = values_[kBgmVolume] * kVolumeStep;
  g_application.sound->SetGroupVolume(g_global.music_volume, 1);
  g_global.effect_volume = values_[kSeVolume] * kVolumeStep;
  g_application.sound->SetGroupVolume(g_global.effect_volume, 0);
  g_fn_game.set_bgm_mode(values_[kBgmMode]);
  if (values_[kStageReplay]) {
    g_global.flags |= cGlobal::kFlagStageReplays;
  } else {
    g_global.flags &= ~cGlobal::kFlagStageReplays;
  }
  if (values_[kPause]) {
    g_global.flags |= cGlobal::kFlagPause;
  } else {
    g_global.flags &= ~cGlobal::kFlagPause;
  }
  if (values_[kBossName]) {
    g_global.flags |= cGlobal::kFlagEnglish;
  } else {
    g_global.flags &= ~cGlobal::kFlagEnglish;
  }
  g_global.rules.time_limit =
      static_cast<short>(kStartTimes[values_[kStartTime]]);
  g_global.rules.stock = values_[kShips];
  switch (values_[kTimerMode]) {
    case 0:
      g_global.rules.mode = 0;
      break;
    case 1:
      g_global.rules.mode = 1;
      break;
  }
  switch (values_[kExtend]) {
    case 0:
      g_global.rules.extend = GameRules::kExtendNone;
      break;
    case 1:
      g_global.rules.extend = GameRules::kExtendBoss;
      break;
    case 2:
      g_global.rules.extend = GameRules::kExtendParts;
      break;
  }
  g_score_table.SetRules(&g_global.rules);
  if (!g_score_table.Load()) {
    g_score_table.Reset();
  }
  if (values_[kScoreReset]) {
    g_score_table.Reset();
    g_score_table.Store();
    values_[kScoreReset] = 0;
  }
  g_score_table.ClearRank();
}

// 0x41c8a0
void FnOption::SetPreset(int preset) {
  const int* rules = presets_[preset];
  int custom = preset == kCustomPreset;
  for (int i = 0; i < kRules; ++i) {
    enabled_[kFirstRuleItem + i] = custom;
    values_[kFirstRuleItem + i] = rules[i];
  }
}

// 0x41ca50
void HLSTxt::SetText(const char* text) { strcpy(text_, text); }

// 0x41caf0
void FnOption::LoadSettings() {
  LANGID language = GetUserDefaultLangID();
  Profile& profile = g_application.profile;
  profile.SetSection("Option");
  values_[kBgmVolume] = profile.GetInt("BGMVolume", 8);
  values_[kSeVolume] = profile.GetInt("SEVolume", 8);
  values_[kBgmMode] = profile.GetInt("BGMMode", 0);
  values_[kStageReplay] = profile.GetBool("RecAllStage", 0);
  values_[kGameMode] = profile.GetInt("GameMode", 0);
  values_[kPause] = profile.GetBool("AppPause", 0);
  values_[kBossName] = profile.GetBool("NameEnglish", language != 0x411);
  int* custom = presets_[kCustomPreset];
  custom[0] = profile.GetInt("CustomStartTimer", custom[0]);
  custom[1] = profile.GetInt("CustomTimerMode", custom[1]);
  custom[2] = profile.GetInt("CustomStartPlayer", custom[2]);
  custom[3] = profile.GetInt("CustomPlayerExtend", custom[3]);
  if (custom[0] == 0 && custom[2] == 0) {
    custom[2] = 1;
  }
  const int* preset = presets_[values_[kGameMode]];
  for (int i = 0; i < kRules; ++i) {
    values_[kFirstRuleItem + i] = preset[i];
  }
  profile.SetSection("DirectX");
  values_[kDialog] = profile.GetBool("ViewDialog", 1);
  values_[kScoreReset] = 0;
  values_[kExit] = 0;
  values_[kShotButton] = Trigger(kShotButton);
  values_[kSlowButton] = Trigger(kSlowButton);
  values_[kWeaponButton] = Trigger(kWeaponButton);
  values_[kPauseButton] = Trigger(kPauseButton);
  Apply();
}

// 0x41cfc0
void FnOption::SaveSettings() {
  Profile& profile = g_application.profile;
  profile.SetSection("Option");
  profile.WriteInt("BGMVolume", values_[kBgmVolume], "%d");
  profile.WriteInt("SEVolume", values_[kSeVolume], "%d");
  profile.WriteInt("BGMMode", values_[kBgmMode], "%d");
  profile.WriteBool("RecAllStage", values_[kStageReplay]);
  profile.WriteBool("AppPause", values_[kPause]);
  profile.WriteBool("NameEnglish", values_[kBossName]);
  profile.WriteInt("GameMode", values_[kGameMode], "%d");
  int* custom = presets_[kCustomPreset];
  profile.WriteInt("CustomStartTimer", custom[0], "%d");
  profile.WriteInt("CustomTimerMode", custom[1], "%d");
  profile.WriteInt("CustomStartPlayer", custom[2], "%d");
  profile.WriteInt("CustomPlayerExtend", custom[3], "%d");
  profile.SetSection("DirectX");
  profile.WriteBool("ViewDialog", values_[kDialog]);
}

// 0x41d340
inline void FnOption::UpdateButtonText(int item) {
  char name[4];
  g_application.input->pad(0)->GetName(values_[item], name);
  strncpy(items_[item]->text() + kOptionItems[item].column, name, 2);
}

// 0x41d390
void FnOption::UpdateChoiceText(int item) {
  char text[32];
  int value = values_[item];
  const char* name = "INFINITY";
  switch (item) {
    case kGameMode:
      name = kModeNames[value];
      break;
    case kStartTime:
      if (value != 0) {
        sprintf(text, "%d SECONDS", kStartTimes[value]);
        name = text;
      }
      break;
    case kTimerMode:
      name = kTimerModeNames[value];
      break;
    case kShips:
      if (value >= 2) {
        sprintf(text, "%d SHIPS", value);
        name = text;
      } else if (value != 0) {
        sprintf(text, "1 SHIP");
        name = text;
      }
      break;
    case kExtend:
      name = kExtendNames[value];
      break;
  }
  const char* left = "  ";
  const char* right;
  if (enabled_[item]) {
    if (value != 0) {
      left = "< ";
    }
    right = value != kOptionItems[item].count - 1 ? " >" : "";
  } else {
    right = "";
  }
  sprintf(items_[item]->text(), kOptionItems[item].text, left, name, right);
  kOptionItems[item].width = static_cast<short>(strlen(name) * 8);
}

// 0x41d4d0
void FnOption::Move() {
  blink_ -= 4;
  if (blink_ <= 0) {
    blink_ = 255;
  }
  if (g_global.screen_fade->fade_frames() != 0) {
    return;
  }
  if (flags_ & kFlagExit) {
    g_application.user_func->Finish(&g_fn_title);
    return;
  }
  HLInput* input = g_application.input;
  unsigned short pressed = g_application.input->pad(0)->pressed();
  if (pressed & (HLInpDev::kUp | HLInpDev::kDown)) {
    if (pressed & HLInpDev::kUp) {
      do {
        if (--cursor_ < 0) {
          cursor_ = kItems - 1;
        }
      } while (!enabled_[cursor_]);
    } else {
      do {
        if (++cursor_ >= kItems) {
          cursor_ = 0;
        }
      } while (!enabled_[cursor_]);
    }
    PlayEffect(kSndFireMe);
    blink_ = 255;
    flags_ |= kFlagHeld;
    return;
  }
  int cursor = cursor_;
  switch (kOptionItems[cursor].kind) {
    case kKindChoice:
    case kKindFormatted: {
      short old = values_[cursor_];
      if (pressed & HLInpDev::kLeft) {
        if (--values_[cursor_] < 0) {
          values_[cursor_] = 0;
        }
      } else if (pressed & HLInpDev::kRight) {
        if (++values_[cursor_] >= kOptionItems[cursor_].count) {
          values_[cursor_] = kOptionItems[cursor_].count - 1;
        }
      }
      if (old == values_[cursor_]) {
        break;
      }
      switch (cursor_) {
        case kBgmVolume:
          if (!(flags_ & kFlagMusic) && g_bgm.loaded()) {
            g_application.sound->channel(kChannelBgm)
                ->Assign(&g_bgm, Channel::kLoop, 1.0f);
            flags_ |= kFlagMusic;
          }
          g_global.SetMusicVolume(values_[cursor_] * kVolumeStep);
          break;
        case kSeVolume:
          g_global.SetEffectVolume(values_[cursor_] * kVolumeStep);
          break;
        case kGameMode:
          SetPreset(values_[cursor_]);
          break;
        case kStartTime:
          if (values_[cursor_] == 0 && values_[kShips] == 0) {
            values_[kShips] = 1;
          }
          break;
        case kShips:
          if (values_[cursor_] == 0 && values_[kStartTime] == 0) {
            values_[kStartTime] = 1;
          }
          break;
      }
      if (kOptionItems[cursor_].kind == kKindFormatted) {
        UpdateChoiceText(cursor_);
        int* preset = presets_[values_[kGameMode]];
        for (int i = 0; i < kRules; ++i) {
          preset[i] = values_[kFirstRuleItem + i];
          UpdateChoiceText(kFirstRuleItem + i);
        }
      }
      PlayEffect(kSndSwitch);
      break;
    }
    case kKindAction:
      if (pressed & (HLInpDev::kTrigger * 0xb)) {
        if (cursor == kExit) {
          g_application.sound->channel(kChannelBgm)->Fade(1.0f, 0, 1.0f, 1);
          g_global.screen_fade->FadeOut(60);
          flags_ |= kFlagExit;
        }
        PlayEffect(kSndSwitch);
      }
      break;
    case kKindButton: {
      int code = input->pad(0)->GetPressed();
      if (code >= 0) {
        if (flags_ & kFlagHeld) {
          break;
        }
        flags_ |= kFlagHeld;
        PlayEffect(kSndSwitch);
        if (code != values_[cursor_]) {
          g_application.input->pad(0)->set_trigger(kOptionItems[cursor_].param,
                                                   code);
          values_[cursor_] = code;
          UpdateButtonText(cursor_);
        }
      } else {
        flags_ &= ~kFlagHeld;
      }
      break;
    }
  }
}

// 0x41d880
void FnOption::Draw() {
  g_application.d3d->ClearTexture();
  Transform transform;
  SetIdentity(&transform);
  ScaleTransform(&transform, 65, 65);
  RotateTransform(&transform, g_application.frame_count * 3);
  TranslateTransform(&transform, 300, 320);
  g_application.d3d->DrawPoints(D3DPT_LINESTRIP, &transform, g_hexagon,
                                0x6400ff80, 7);

  SetIdentity(&transform);
  D3DVECTOR quad[4];
  quad[0].z = quad[1].z = quad[2].z = quad[3].z = 0.5f;
  float y = 140;
  for (int i = 0; i < kItems; ++i) {
    if (kOptionItems[i].count != 0 && enabled_[i]) {
      quad[0].x = quad[2].x = static_cast<float>(
          values_[i] * kOptionItems[i].param + kOptionItems[i].column + 464);
      quad[1].x = quad[3].x = kOptionItems[i].width + quad[2].x;
      quad[0].y = quad[1].y = y;
      quad[2].y = quad[3].y = y + 8;
      g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, &transform, quad,
                                    0x6400ff80, 4);
    }
    y += 16;
  }

  int cursor = cursor_;
  const char* text = items_[cursor]->text();
  quad[0].x = 320;
  quad[1].x = strlen(text) * 8 + 320.0f;
  quad[0].y = quad[1].y = cursor * 16.0f + 148;
  g_application.d3d->DrawPoints(D3DPT_LINELIST, &transform, quad,
                                (blink_ << 24) | 0xffdd00, 2);

  y = 140;
  for (int i = 0; i < kItems; ++i) {
    items_[i]->PrintInline(320, static_cast<int>(y), 0x80ffffff, -1);
    y += 16;
  }

  SetIdentity(&transform);
  TranslateTransform(
      &transform,
      -static_cast<float>(
          static_cast<unsigned int>(g_application.frame_count & 0x3f) * 2),
      450);
  SetTexture(g_application.d3d, &g_fonts.textures[2], 0);
  g_application.d3d->DrawTexels(D3DPT_TRIANGLESTRIP, &transform, kTitleBar,
                                kTitleBarTexels, 0xc800ff80, 4);
}

// 0x41dc60
int FnOption::Initialize() {
  g_global.ClearStage();
  BackgroundGrid* background = new BackgroundGrid;
  background_ = background;
  if (background == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  background->app_ = &g_application;
  background->color_ = 0xff004030;
  background->speed_ = 8.0f;
  g_application.tasks.AddBack(background_, 2);
  SetPreset(values_[kGameMode]);
  for (int i = 0; i < kItems; ++i) {
    items_[i] = g_fonts.small_text.Create(0x40, NULL);
    if (kOptionItems[i].kind == kKindFormatted) {
      UpdateChoiceText(i);
    } else {
      strcpy(items_[i]->text(), kOptionItems[i].text);
    }
  }
  cursor_ = kExit;
  flags_ = 0;
  blink_ = 0;
  UpdateButtonText(kShotButton);
  UpdateButtonText(kSlowButton);
  UpdateButtonText(kWeaponButton);
  UpdateButtonText(kPauseButton);
  g_global.screen_fade->FadeIn(60);
  return 1;
}

// 0x41ded0
void FnOption::Finalize() {
  Apply();
  SaveSettings();
  for (int i = 0; i < kItems; ++i) {
    if (items_[i] != NULL) {
      items_[i]->Release();
      items_[i] = NULL;
    }
  }
  if (background_ != NULL) {
    g_application.tasks.Delete(background_);
    background_ = NULL;
  }
}
