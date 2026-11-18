// Untextured sprites: the ring of a shockwave and the player's laser.
#ifndef WF_LINE_SPRITES_H_
#define WF_LINE_SPRITES_H_

#include "hls_anm.h"

// The ring of a shockwave: an untextured triangle (vtable 0x4527d0).
class ShockwaveSprite : public HLSObject {
 public:
  ShockwaveSprite() {}  // 0x410530
  // Draws the triangle in `color`; `blend` 1 adds it to the screen.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x415120
};

extern ShockwaveSprite g_shockwave;  // 0x46904c

// The player's laser (weapon 1): a fan from the ship toward its tip, which
// trails behind the ship while the fire button is up (vtable 0x452860).
class LaserSprite : public HLSObject {
 public:
  enum { kMaxLength = 140 };

  LaserSprite();  // 0x4153c0
  // Draws the fan dimmed and its outline in `color`; `blend` 1 adds it to
  // the screen.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x415400

  // Puts the tip straight above the ship at (`x`, `y`).
  void Reset(float x, float y);  // 0x415220
  // Follows the ship to (`x`, `y`): the tip moves with it while `hold`,
  // else it is pushed by (`push_x`, `push_y`) and pulled within
  // kMaxLength of the ship.
  void Move(float x, float y, int hold, float push_x,
            float push_y);  // 0x415250

  short angle() const { return angle_; }
  short spread() const { return spread_; }

 private:
  friend class Control01;

  D3DVECTOR points_[3];  // 0x08, relative to the ship
  float tip_x_;          // 0x2c
  float tip_y_;          // 0x30
  float x_;              // 0x34, of the ship
  float y_;              // 0x38
  short angle_;          // 0x3c, from the ship to the tip
  short spread_;         // 0x3e, half the angle of the fan
};

extern LaserSprite g_laser;  // 0x4690f0

#endif  // WF_LINE_SPRITES_H_
