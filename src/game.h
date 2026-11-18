// Game: the state of the running game (vtable 0x4514a4, g_game): the
// clock, the time bonus and the player's stock.
#ifndef WF_GAME_H_
#define WF_GAME_H_

#include "game_rules.h"

class Actor;

class Game {
 public:
  enum Flags {
    kCountUp = 0x1,    // no time limit: the clock counts up
    kTimeBonus = 0x2,  // destroyed bosses give time, misses take it
    kRunning = 0x4,    // the clock is running
    kGameOver = 0x8,
  };
  enum { kMaxTimer = 999 * 60 };

  Game();  // 0x41ec90
  // scalar deleting dtor 0x401a10
  virtual ~Game() {}

  // Fills `rules` with the default rule set.
  void GetDefaultRules(GameRules* rules);  // 0x41eca0
  // Starts a game under `rules`.
  void Start(const GameRules* rules);  // 0x41ecc0
  // Restores the stock and extend counter (when they are in use).
  void SetLives(short lives, short extend_count);  // 0x41ed50
  // Advances the clock by one frame.
  void Tick();  // 0x41ed80
  // Spawns the player, using up a life; ends the game without one.
  void SpawnPlayer();        // 0x41ee00
  void OnPlayerDestroyed();  // 0x41ee60
  void OnPartDestroyed();    // 0x41ee80
  void OnBossDestroyed();    // 0x41eec0

 private:
  unsigned int flags_;  // 0x04
  int timer_;           // 0x08, frames left (elapsed with kCountUp)
  int bonus_;           // 0x0c, frames still to add to the clock
  int frames_;          // 0x10, frames played
  short lives_;         // 0x14, -1 without stock
  short extend_count_;  // 0x16, parts destroyed toward the next extend
  GameRules rules_;     // 0x18
};

extern Game g_game;      // 0x4675fc
extern Actor* g_player;  // 0x467324
// The boss actor (control 20) while one is alive.
extern Actor* g_boss;  // 0x467320

#endif  // WF_GAME_H_
