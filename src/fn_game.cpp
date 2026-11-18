// fnGame: the stage that runs a game.
#include "fn_game.h"

#include "actor.h"
#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "fn_title.h"
#include "game.h"
#include "game_hud.h"
#include "hl_input.h"
#include "replay.h"
#include "snd_file_seq.h"
#include "snd_sys.h"
#include "sounds.h"
#include "sprite.h"

// 0x41be00
FnGame::FnGame() {
  name_ = "fnGame";
  unknown_14_ = NULL;
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

// 0x41c0e0
void FnGame::Finalize() {
  g_replay.Close();
  g_application.input->set_time_stop_mask(0);
  g_actor_system.DestroyAll();
  g_global.ResetSound();
  g_application.tasks.Delete(unknown_14_);
  unknown_14_ = NULL;
  g_application.overlays.Delete(hud_);
  hud_ = NULL;
}
