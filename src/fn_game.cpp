// fnGame: the stage that runs a game.
#include "fn_game.h"

#include "actor_system.h"
#include "replay.h"

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
