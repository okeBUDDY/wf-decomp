// GameConfig: the settings of the [Option] section of the profile
// (g_config). Only its functions are known so far.
#ifndef WF_GAME_CONFIG_H_
#define WF_GAME_CONFIG_H_

class GameConfig {
 public:
  // Reads the settings from the profile.
  void Load();  // 0x41caf0
};

extern GameConfig g_config;  // 0x467cd0

#endif  // WF_GAME_CONFIG_H_
