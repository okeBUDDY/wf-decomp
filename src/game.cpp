#include "game.h"

#include "actor_system.h"

// 0x41ec90
Game::Game() {}

// 0x41eca0
void Game::GetDefaultRules(GameRules* rules) {
  rules->mode = 0;
  rules->time_limit = 180;
  rules->stock = 0;
  rules->extend = GameRules::kExtendNone;
}

// 0x41ecc0
void Game::Start(const GameRules* rules) {
  rules_ = *rules;
  flags_ = 0;
  extend_count_ = -1;
  if (rules_.time_limit != 0) {
    if (rules_.mode == 0) {
      flags_ = kTimeBonus;
    }
    timer_ = rules_.time_limit * 60;
  } else {
    flags_ = kCountUp;
    if (rules_.stock == 0) {
      rules_.stock = 1;
    }
    if (rules_.extend == GameRules::kExtendParts) {
      extend_count_ = 0;
    }
    timer_ = 0;
  }
  lives_ = rules_.stock > 0 ? rules_.stock : -1;
  bonus_ = 0;
  frames_ = 0;
}

// 0x41ed50
void Game::SetLives(short lives, short extend_count) {
  if (lives_ >= 0) {
    lives_ = lives;
  }
  if (extend_count_ >= 0) {
    extend_count_ = extend_count;
  }
}

// 0x41ed80
void Game::Tick() {
  if (flags_ & kCountUp) {
    if (flags_ & kRunning) {
      ++timer_;
      ++frames_;
    }
  } else {
    if (flags_ & kRunning) {
      --timer_;
      ++frames_;
    }
    if ((flags_ & kTimeBonus) && bonus_ != 0) {
      if (bonus_ > 0) {
        bonus_ -= 20;
        timer_ += 20;
      } else {
        bonus_ += 20;
        timer_ -= 20;
      }
    }
    if (timer_ <= 0) {
      timer_ = 0;
      flags_ |= kGameOver;
    }
  }
  if (timer_ > kMaxTimer) {
    timer_ = kMaxTimer;
  }
}

// 0x41ee00
void Game::SpawnPlayer() {
  if (rules_.stock != 0) {
    if (lives_ <= 0) {
      flags_ |= kGameOver;
      if (extend_count_ > 0) {
        extend_count_ = 0;
      }
      return;
    }
    --lives_;
  } else if (flags_ & kGameOver) {
    return;
  }
  g_player = g_actor_system.Create(-1, 0, 320.0f, 400.0f, 8, 0, 0);
}

// 0x41ee60
void Game::OnPlayerDestroyed() {
  if (flags_ & kTimeBonus) {
    bonus_ -= 20 * 60;
  }
  g_player = NULL;
}

// 0x41ee80
void Game::OnPartDestroyed() {
  if (rules_.stock != 0 && rules_.extend == GameRules::kExtendParts) {
    if (++extend_count_ == kPartsPerExtend) {
      if (lives_ < kMaxLives) {
        ++lives_;
      }
      extend_count_ = 0;
    }
  }
}

// 0x41eec0
void Game::OnBossDestroyed() {
  if (flags_ & kTimeBonus) {
    bonus_ += 30 * 60;
  }
  if (rules_.stock != 0 && rules_.extend == GameRules::kExtendBoss) {
    if (lives_ < kMaxLives) {
      ++lives_;
    }
  }
}
