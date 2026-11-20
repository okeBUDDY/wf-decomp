// Sprite objects and their animations: HLSObject is the interface of
// everything the sprite code draws; HLSAnm plays a script of objects
// (HLSAnmSys owns the pool, vtable 0x451cc4).
#ifndef WF_HLS_ANM_H_
#define WF_HLS_ANM_H_

#include <windows.h>

#include <d3d.h>

#include "system.h"

struct Application;
class HLSHitImage;
class HLSPatFile;
class HLSurface;
struct Transform;

// vtable 0x451ca4.
class HLSObject {
 public:
  HLSObject() { texture_ = NULL; }
  virtual ~HLSObject() {}  // 0x407a60
  virtual void Reset() {}
  virtual void Draw(const Transform* transform, D3DCOLOR color, int blend) = 0;
  // Marks the object in `image` with `id`.
  virtual void Hit(HLSHitImage* image, int x, int y, DWORD id) {}  // 0x407360

 protected:
  friend class ActorSystem;
  friend class HLSAnmSys;
  HLSurface* texture_;  // 0x04, used by HLSPat
};

// One animation: a script of (object, frames) pairs. -2 entries jump
// (target, count), -1 ends.
struct AnmData {
  HLSPatFile* pat_file;  // 0x00, the patterns the script numbers
  short* script;         // 0x04
};

// vtable 0x451cb4.
class HLSAnm : public HLSObject {
 public:
  enum { kJump = -2, kEnd = -1 };

  HLSAnm() {
    data_ = NULL;
    cursor_ = NULL;
    object_ = NULL;
    wait_ = loop_ = 0;
  }  // 0x407390
  virtual void Reset();  // 0x4073b0
  // Draws the current object, then steps the script while the time moves.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x4073e0
  virtual void Hit(HLSHitImage* image, int x, int y,
                   DWORD id);  // 0x4073c0

 private:
  friend class HLSAnmSys;

  // Counts down the current entry and moves to the next one.
  void Step();  // 0x407420

  AnmData* data_;      // 0x08
  short* cursor_;      // 0x0c, the current script entry
  HLSObject* object_;  // 0x10
  short wait_;         // 0x14, frames left on the entry
  short loop_;         // 0x16, jumps left, -1: forever
};

class HLSAnmSys : public System {
 public:
  HLSAnmSys();                               // 0x4074f0
  virtual int Initialize(Application* app);  // 0x407520
  virtual void Finalize();                   // 0x4072f0
  virtual void Pause() {}
  virtual void Resume() {}

  // Starts `data` on a free animation; NULL when none is free.
  HLSAnm* Play(AnmData* data);  // 0x410850
  // The number of animations (before Initialize()).
  void set_count(int count) { count_ = count; }

 private:
  int count_;         // 0x08
  Application* app_;  // 0x0c
  HLSAnm* anims_;     // 0x10, new[]ed
};

extern HLSAnmSys g_anm_sys;  // 0x4674b4

#endif  // WF_HLS_ANM_H_
