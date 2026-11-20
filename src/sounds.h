// The sound effects the game loads at startup (cGlobal::LoadSounds()
// 0x401550). Sound i plays on sound channel i.
#ifndef WF_SOUNDS_H_
#define WF_SOUNDS_H_

#include "snd_buf.h"
#include "snd_strm.h"

enum SoundId {
  kSndBomb1 = 0,     // "bomb_1.ogg"
  kSndBomb2 = 1,     // "bomb_2.ogg"
  kSndBomb3 = 2,     // "bomb_3.ogg"
  kSndBomb4 = 3,     // "bomb_4.ogg"
  kSndBombPre = 4,   // "bomb_pre.ogg"
  kSndBombAll = 5,   // "bomb_all.ogg"
  kSndBombMe = 6,    // "bomb_me.ogg"
  kSndDamage = 7,    // "damage.ogg"
  kSndNoDamage = 8,  // "nodamage.ogg"
  kSndWarning = 10,  // "warning.ogg"
  kSndSwitch = 12,   // "switch.ogg"
  kSndWeapon0 = 13,  // "wep0.ogg"
  kSndWeapon1 = 14,  // "wep1.ogg"
  kSndFireMe = 16,   // "fire_me.ogg"
  kSndFireBl = 17,   // "fire_bl.ogg"
  kSndFireCn = 18,   // "fire_cn.ogg"
  kSndFireLs1 = 19,  // "fire_ls1.ogg"
  kSndFireLs2 = 20,  // "fire_ls2.ogg"
  kSndFireLs3 = 21,  // "fire_ls3.ogg"
  kSndFireMs = 22,   // "fire_ms.ogg"
  kSndBarrier = 24,  // "barrier.ogg"
  kSndBoost1 = 25,   // "boost1.ogg"
  kSndBoost2 = 26,   // "boost2.ogg"
  kSndTrans1 = 27,   // "trans1.ogg"
  kSndTrans2 = 28,   // "trans2.ogg"
  kSounds = 32,
  kChannelBgm = kSounds,  // the channel of g_bgm
};

// The volume groups of the sounds (CSndBuf::group()).
enum SoundGroup {
  kSoundEffects = 0,  // g_sounds
  kSoundMusic = 1,    // g_bgm
};

extern CSndBuf g_sounds[kSounds];  // 0x4681f0
// The music: "bgm.seq", else "bgm.ogg", else "bgm.wav".
extern CSndStrm g_bgm;  // 0x4686f0

#endif  // WF_SOUNDS_H_
