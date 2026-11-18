// HLSHitImage: a low resolution 16-bit image the sprites mark with their
// ids to find what is under a point (vtable 0x451fe0).
#ifndef WF_HLS_HIT_IMAGE_H_
#define WF_HLS_HIT_IMAGE_H_

#include <string.h>

#include "object.h"

class HLSurface;

class HLSHitImage : public Object {
 public:
  enum { kMaxHeight = 120 };

  HLSHitImage();  // 0x4078d0
  // Allocates the image for a `width` x `height` screen, halved until it
  // is at most kMaxHeight high. Returns 0 without memory.
  int Create(int width, int height);  // 0x407650
  void Release();                     // 0x4076e0
  // Fills the rectangle (`x` +- `width`, `y` +- `height`) (screen
  // coordinates) with `id`.
  void Fill(int x, int y, int width, int height,
            unsigned long id);  // 0x407720
  // Clears the image to 0 (no actor).
  void Clear() { memset(bits_, 0, size_); }

 private:
  friend class Actor;

  unsigned short* bits_;  // 0x08, malloc()ed
  HLSurface* surface_;    // 0x0c
  int size_;              // 0x10, bytes
  short width_;           // 0x14
  short height_;          // 0x16
  short shift_;           // 0x18, screen to image: >> shift_
};

// The hit image of the actors.
extern HLSHitImage g_hit_image;  // 0x468ee0

#endif  // WF_HLS_HIT_IMAGE_H_
