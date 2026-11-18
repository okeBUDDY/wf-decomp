// cGlobal: the game itself (g_global, a System named "cGlobal"): it loads
// the data, runs the title, the stages and the results, and keeps the
// state of the current game.
#ifndef WF_CGLOBAL_H_
#define WF_CGLOBAL_H_

#include "system.h"

class cGlobal : public System {
 public:
  cGlobal();                                 // 0x4010a0
  virtual int Initialize(Application* app);  // 0x401880
  virtual void Finalize();                   // 0x4010d0
  virtual void Pause() {}
  virtual void Resume() {}

  int unknown_08_;      // 0x08
  int stage;            // 0x0c, the current stage (from 0)
  int unknown_10_[2];   // 0x10
  int score;            // 0x18
  int unknown_1c_;      // 0x1c
  short unknown_20_;    // 0x20
  short deaths;         // 0x22, in this game
  int unknown_24_;      // 0x24
  void* unknown_28_;    // 0x28
  float music_volume;   // 0x2c
  float effect_volume;  // 0x30
};

extern cGlobal g_global;  // 0x467538

#endif  // WF_CGLOBAL_H_
