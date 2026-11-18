// GameRules: the rule set chosen on the title screen; the score file keeps
// one table per rule set.
#ifndef WF_GAME_RULES_H_
#define WF_GAME_RULES_H_

struct GameRules {
  enum Extend { kExtendNone, kExtendBoss, kExtendParts };

  short mode;        // 0: time bonuses are given
  short time_limit;  // seconds, 0: no limit (180 by default)
  short stock;       // player stock, 0: none
  short extend;      // Extend
};

#endif  // WF_GAME_RULES_H_
