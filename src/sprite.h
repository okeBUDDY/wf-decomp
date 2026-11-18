// Sprite: a textured quad drawn as four pre-transformed vertices.
#ifndef WF_SPRITE_H_
#define WF_SPRITE_H_

// Layout of D3DFVF_TLVERTEX (Direct3D 7).
struct TLVertex {
  float x;
  float y;
  float z;
  float rhw;
  unsigned long color;
  unsigned long specular;
  float tu;
  float tv;
};

class Sprite {
 public:
  // 0x401000: sets the colour (0-255 per channel) of all four corners.
  void SetColor(int r, int g, int b, int a);
  // Fades from the current colour to (`r`, `g`, `b`, `a`) in `frames`.
  void FadeTo(short frames, int r, int g, int b, int a);  // 0x41bca0

 private:
  char unknown00_[0x18];  // not recovered yet
  TLVertex vertices_[4];  // 0x18
  short red_;             // 0x98
  short green_;           // 0x9a
  short blue_;            // 0x9c
  short alpha_;           // 0x9e
  char unknown_a0_[0xb0 - 0xa0];
  short color_flag_b0_;  // 0xb0, cleared by SetColor; meaning unknown
};

#endif  // WF_SPRITE_H_
