// HLWindow: creates the game window and runs its window procedure
// (vtable 0x452130).
#ifndef WF_HL_WINDOW_H_
#define WF_HL_WINDOW_H_

#include <windows.h>

#include "system.h"

struct Application;

class HLWindow : public System {
 public:
  // Receives mouse events; returns nonzero when the event was handled.
  typedef int (*MouseFunc)(int event, int x, int y, int buttons, void* user);
  enum MouseEvent {
    kClick = 1,
    kRightClick = 2,
    kDoubleClick = 3,
    kDragStart = 5,
    kDrag = 6,
    kDragEnd = 7,
  };
  enum MouseFlags {
    kCaptured = 0x1,
    kRightDown = 0x2,
    kDragging = 0x4,
  };

  HLWindow();                                // 0x40cdd0
  virtual int Initialize(Application* app);  // 0x40d230
  virtual void Finalize();                   // 0x40ce30
  virtual void Pause() {}
  virtual void Resume() {}

  void set_title(const char* title) { title_ = title; }
  // Routes mouse events to `func` (NULL: nobody), ending a drag first.
  void SetMouseHandler(MouseFunc func, void* user);  // 0x40ca90

 private:
  static int CALLBACK NoWordBreak(LPSTR text, int start, int length,
                                  int code);  // 0x40cdc0
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);  // 0x40cf30
  // Turns mouse messages into MouseEvents; returns the handler's result.
  int OnMouse(UINT msg, WPARAM wparam, LPARAM lparam);  // 0x40cb30

  HWND message_field_;        // 0x08
  HFONT fixed_font_;          // 0x0c
  HFONT gothic_font_;         // 0x10
  HFONT pgothic_font_;        // 0x14
  const char* title_;         // 0x18
  int icon_;                  // 0x1c, resource id, 0: default icon
  int menu_;                  // 0x20, resource id, 0: no menu
  short message_height_;      // 0x24, height of the message field
  Application* app_;          // 0x28
  void* buffer_;              // 0x2c, malloc()ed
  MouseFunc mouse_func_;      // 0x30
  void* mouse_user_;          // 0x34
  unsigned int mouse_flags_;  // 0x38
  short press_x_;             // 0x3c
  short press_y_;             // 0x3e
};

#endif  // WF_HL_WINDOW_H_
