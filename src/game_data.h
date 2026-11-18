// The pattern files the game loads at startup (LoadGameData() 0x401470).
#ifndef WF_GAME_DATA_H_
#define WF_GAME_DATA_H_

#include "hls_pat.h"

// One global object: its implicit ctor (0x401bf0) and dtor (0x401e00) are
// out of line, and each one-element array is built with `vector ctor'.
struct PatFiles {
  HLSPatFile sys[1];    // 0x00, "sys0"
  HLSPatFile enemy[1];  // 0x38, "enm0"
  HLSPatFile add[1];    // 0x70, "add0" (drawn additively)
  HLSPatFile etc[3];    // 0xa8, "etc0".."etc2"
};

extern PatFiles g_pats;  // 0x467d70

// The pattern file `index` of plan.dat: it numbers them from enm0 on, and
// enm0, add0 and etc0..etc2 follow one another in memory.
inline HLSPatFile* PlanPatFile(int index) { return &g_pats.enemy[0] + index; }

#endif  // WF_GAME_DATA_H_
