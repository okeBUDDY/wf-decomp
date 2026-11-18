// Sprite patterns: HLSPat draws a rectangle of a texture as a quad;
// HLSPatSys owns the pool of patterns and HLSPatFile the patterns loaded
// with one texture.
#ifndef WF_HLS_PAT_H_
#define WF_HLS_PAT_H_

#include <windows.h>

#include <d3d.h>

#include "hl_3d.h"
#include "hl_surface.h"
#include "hls_anm.h"

class HLSPatSys;

// vtable 0x451ec8.
class HLSPat : public HLSObject {
 public:
  HLSPat() {
    used_ = 0;
    owns_texture_ = false;
    empty_ = false;
    points_[0].z = points_[1].z = points_[2].z = points_[3].z = 0.5f;
  }  // 0x409c10
  virtual ~HLSPat() { Release(); }  // 0x40a2b0, vector deleting dtor 0x40a970
  // Draws the quad unless it is empty or `color` is transparent; blend 1
  // adds it to the screen.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x409e20
  // Marks the hit rectangle around (`x`, `y`) in `image`.
  virtual void Hit(HLSHitImage* image, int x, int y, DWORD id);  // 0x409c40

  // Uses the `width` x `height` texels at (`u`, `v`) of `texture` (a new
  // texture of that size when NULL), with its origin at (`x`, `y`) and a
  // hit rectangle of `hit_width` x `hit_height`. Returns 0 on failure.
  int Setup(int width, int height, HLSurface* texture, short u, short v, int x,
            int y, int hit_width, int hit_height);  // 0x409c70
  void Release() {
    if (owns_texture_) {
      texture_->Release();
      delete texture_;
    }
    texture_ = NULL;
    owns_texture_ = false;
    empty_ = false;
    used_ = 0;
  }

  short used() const { return used_; }

  static HL3D* d3d_;          // 0x468efc
  static HLSPatSys* system_;  // 0x468f00

 private:
  D3DVECTOR points_[4];        // 0x08, the corners of the quad
  TexCoord coords_[4];         // 0x38
  short used_;                 // 0x58
  bool owns_texture_;          // 0x5a
  bool empty_;                 // 0x5b, nothing to draw
  unsigned short hit_width_;   // 0x5c, half the hit width
  unsigned short hit_height_;  // 0x5e
};

// vtable 0x451ed8.
class HLSPatSys : public System {
 public:
  HLSPatSys() {
    app_ = NULL;
    pats_ = NULL;
    name_ = "HLSPatSys";
    count_ = 0x100;
  }  // 0x40a0e0
  virtual int Initialize(Application* app);  // 0x40a890
  virtual void Finalize();                   // 0x40a110
  virtual void Pause() {}
  virtual void Resume() {}

  // Sets up a free pattern (see HLSPat::Setup()); NULL when none is left.
  HLSPat* Create(int width, int height, HLSurface* texture, short u, short v,
                 int x, int y, int hit_width, int hit_height);  // 0x409f80

 private:
  int count_;         // 0x08
  Application* app_;  // 0x0c
  HLSPat* pats_;      // 0x10, new[]ed
};

// vtable 0x4514b4.
class HLSPatFile : public Object {
 public:
  HLSPatFile();  // 0x40a000
  // 0x401ae0, scalar deleting dtor 0x401b10
  virtual ~HLSPatFile() {
    if (pats_ != NULL) {
      free(pats_);
      pats_ = NULL;
    }
  }
  // Releases the patterns and the texture.
  void Release();  // 0x40a060
  // The pattern `index`, or NULL before the file is loaded.
  HLSPat* Get(int index) const { return pats_ != NULL ? pats_[index] : NULL; }

 private:
  HLSurface texture_;  // 0x08
  HLSPat** pats_;      // 0x30, malloc()ed
  int count_;          // 0x34
};

#endif  // WF_HLS_PAT_H_
