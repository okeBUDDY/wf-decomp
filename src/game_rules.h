// GameRules: the rule set chosen on the title screen; the score file keeps
// one table per rule set. GameSettings (vtable 0x4514a4, g_game_settings)
// holds the current rules.
#ifndef WF_GAME_RULES_H_
#define WF_GAME_RULES_H_

struct GameRules {
  short mode;        // 0 = normal
  short time_limit;  // seconds (180 by default)
  short stock;
  short option;
};

class GameSettings {
 public:
  // Fills `rules` with the default rule set. 0x41eca0
  void GetDefaultRules(GameRules* rules);
};

extern GameSettings g_game_settings;  // 0x4675fc

#endif  // WF_GAME_RULES_H_
