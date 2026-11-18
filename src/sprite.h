// Sprite: a screen-sized quad drawn as four pre-transformed vertices, a
// task of Application::overlays (vtable 0x453094). g_global.screen_fade
// flashes and fades the screen with it.
#ifndef WF_SPRITE_H_
#define WF_SPRITE_H_

#include "task_list.h"

struct Application;

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

class Sprite : public Task {
 public:
  Sprite();  // 0x41bd30
  // Covers the screen of app_ in transparent black; 0 without app_.
  virtual int Initialize();  // 0x41baa0
  virtual void Finalize() {}
  // Advances the fade; always 1.
  virtual int IsProtected();  // 0x41bb60
  // Draws the quad unless it is transparent.
  virtual void Run();  // 0x41bd60

  // 0x401000: sets the colour (0-255 per channel) of all four corners and
  // stops the fade.
  void SetColor(int r, int g, int b, int a);
  // Fades from the current colour to (`r`, `g`, `b`, `a`) in `frames`.
  void FadeTo(short frames, int r, int g, int b, int a);  // 0x41bca0

  Application* app_;  // 0x14

 private:
  // The colour as a D3DCOLOR.
  unsigned long color() const {
    return (((((alpha_ << 8) | red_) << 8) | green_) << 8) | blue_;
  }

  TLVertex vertices_[4];  // 0x18
  short red_;             // 0x98
  short green_;           // 0x9a
  short blue_;            // 0x9c
  short alpha_;           // 0x9e
  short from_red_;        // 0xa0, the colour the fade starts from
  short from_green_;      // 0xa2
  short from_blue_;       // 0xa4
  short from_alpha_;      // 0xa6
  short delta_red_;       // 0xa8, and how far it goes
  short delta_green_;     // 0xaa
  short delta_blue_;      // 0xac
  short delta_alpha_;     // 0xae
  short fade_frames_;     // 0xb0, 0 while not fading
  short fade_time_;       // 0xb2
};

#endif  // WF_SPRITE_H_
