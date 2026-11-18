// The ring of a shockwave: an untextured triangle (vtable 0x4527d0).
#ifndef WF_SHOCKWAVE_H_
#define WF_SHOCKWAVE_H_

#include "hls_anm.h"

class ShockwaveSprite : public HLSObject {
 public:
  ShockwaveSprite() {}  // 0x410530
  // Draws the triangle in `color`; `blend` 1 adds it to the screen.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x415120
};

extern ShockwaveSprite g_shockwave;  // 0x46904c

#endif  // WF_SHOCKWAVE_H_
