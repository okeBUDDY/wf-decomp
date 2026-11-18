// The pattern files the game loads at startup (LoadGameData() 0x401470).
#ifndef WF_GAME_DATA_H_
#define WF_GAME_DATA_H_

#include "hls_pat.h"

extern HLSPatFile g_sys_pats;     // 0x467d70, "sys0"
extern HLSPatFile g_enemy_pats;   // 0x467da8, "enm0"
extern HLSPatFile g_add_pats;     // 0x467de0, "add0" (drawn additively)
extern HLSPatFile g_etc_pats[3];  // 0x467e18, "etc0".."etc2"

// The pattern file `index` of plan.dat: it numbers them from enm0 on, and
// enm0, add0 and etc0..etc2 follow one another in memory.
inline HLSPatFile* PlanPatFile(int index) { return &g_enemy_pats + index; }

#endif  // WF_GAME_DATA_H_
