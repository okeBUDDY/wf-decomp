// HLSync: paces the frames with a multimedia timer and presents them
// (vtable 0x4520d4).
#ifndef WF_HL_SYNC_H_
#define WF_HL_SYNC_H_

#include <windows.h>

#include "object.h"

struct Application;

class HLSync : public Object {
 public:
  enum Mode {
    kVSync,     // full screen with page flipping: flip
    kWait,      // wait for the timer, then present
    kNoWait,    // present at once
    kSkipDraw,  // present at once; frames are not drawn
  };

  HLSync();  // 0x40c750
  // Starts the timer at fps_ frames per second.
  virtual int Initialize(Application* app);  // 0x40c9e0
  virtual void Finalize();                   // 0x40c6b0
  virtual void Pause() {}
  virtual void Resume() {}

  // Picks the present function for `mode` and the screen mode.
  void SetMode(unsigned char mode);  // 0x40c8e0

 private:
  typedef void (*PresentFunc)(HLSync* sync);

  static void CALLBACK TimeProc(UINT id, UINT msg, DWORD_PTR user,
                                DWORD_PTR dw1, DWORD_PTR dw2);  // 0x40c6e0
  static void Wait(HLSync* sync);                               // 0x40c710
  static void SkipFrame(HLSync* sync);                          // 0x40c720
  static void Flip(HLSync* sync);                               // 0x40c780
  static void WaitBltFast(HLSync* sync);                        // 0x40c7a0
  static void WaitBlt(HLSync* sync);                            // 0x40c7d0
  static void BltFast(HLSync* sync);                            // 0x40c810
  static void Blt(HLSync* sync);                                // 0x40c840
  static void BltFastSkipFrame(HLSync* sync);                   // 0x40c870
  static void BltSkipFrame(HLSync* sync);                       // 0x40c8a0
  static void NoWait(HLSync* sync) {}

  unsigned char mode_;    // 0x08, Mode
  short fps_;             // 0x0a
  Application* app_;      // 0x0c
  UINT timer_;            // 0x10
  volatile char ticked_;  // 0x14, set by every timer tick
  short ticks_;           // 0x16, ticks in the current second
  short frames_;          // 0x18, frames presented in the current second
  short measured_fps_;    // 0x1a, frames presented in the last second
  PresentFunc present_;   // 0x1c
};

#endif  // WF_HL_SYNC_H_
