// fnGame: the stage that runs a game.
#include "fn_game.h"

#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "replay.h"
#include "snd_file_seq.h"
#include "snd_sys.h"
#include "sprite.h"

// 0x41be00
FnGame::FnGame() {
  name_ = "fnGame";
  unknown_14_ = NULL;
  unknown_18_ = NULL;
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
  g_seq_flags[4] = static_cast<unsigned char>(unknown_24_);
}

// 0x41c6a0
void cGlobal::SetMusicVolume(float volume) {
  music_volume = volume;
  g_application.sound->SetGroupVolume(volume, 0);
}

// 0x41c6c0
void cGlobal::SetEffectVolume(float volume) {
  effect_volume = volume;
  g_application.sound->SetGroupVolume(volume, 1);
}
