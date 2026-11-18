#include "hl_window.h"

#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "graphics.h"
#include "hl_sync.h"

static char kName[] = "HLWindow";  // 0x45d32c

// 0x40ca90
void HLWindow::SetMouseHandler(MouseFunc func, void* user) {
  if (app_ == NULL || app_->window == NULL) {
    return;
  }
  if (mouse_func_ != NULL && (mouse_flags_ & kCaptured)) {
    ReleaseCapture();
    if (mouse_flags_ & kDragging) {
      POINT point;
      GetCursorPos(&point);
      ScreenToClient(app_->window, &point);
      mouse_func_(kDrag, point.x, point.y, 0, mouse_user_);
      mouse_func_(kDragEnd, point.x, point.y, 0, mouse_user_);
    }
  }
  mouse_func_ = func;
  mouse_user_ = user;
  mouse_flags_ = 0;
}

// 0x40cb30
int HLWindow::OnMouse(UINT msg, WPARAM wparam, LPARAM lparam) {
  int result = 0;
  switch (msg) {
    case WM_MOUSEMOVE: {
      int x = LOWORD(lparam);
      int y = HIWORD(lparam);
      if (mouse_flags_ & kDragging) {
        result = mouse_func_(kDrag, x, y, wparam, mouse_user_);
      } else if (mouse_flags_ & kCaptured) {
        if (mouse_func_(kDragStart, press_x_, press_y_, wparam, mouse_user_)) {
          mouse_func_(kDrag, x, y, wparam, mouse_user_);
        }
        mouse_flags_ |= kDragging;
        result = 1;
      }
      break;
    }
    case WM_LBUTTONDOWN:
      mouse_flags_ |= kCaptured;
      press_x_ = LOWORD(lparam);
      press_y_ = HIWORD(lparam);
      SetCapture(app_->window);
      break;
    case WM_LBUTTONUP:
      if (mouse_flags_ & kCaptured) {
        int x = LOWORD(lparam);
        int y = HIWORD(lparam);
        ReleaseCapture();
        result = mouse_func_(kClick, x, y, wparam, mouse_user_);
        if (mouse_flags_ & kDragging) {
          result |= mouse_func_(kDrag, x, y, wparam, mouse_user_);
          result |= mouse_func_(kDragEnd, x, y, wparam, mouse_user_);
        }
        mouse_flags_ &= ~(kCaptured | kDragging);
      }
      break;
    case WM_LBUTTONDBLCLK:
      result = mouse_func_(kDoubleClick, LOWORD(lparam), HIWORD(lparam), wparam,
                           mouse_user_);
      break;
    case WM_RBUTTONDOWN:
      mouse_flags_ |= kRightDown;
      break;
    case WM_RBUTTONUP:
      if (mouse_flags_ & kRightDown) {
        result = mouse_func_(kRightClick, LOWORD(lparam), HIWORD(lparam),
                             wparam, mouse_user_);
        mouse_flags_ &= ~kRightDown;
      }
      break;
  }
  return result;
}

// 0x40cda0
void Application::RequestCapture() {
  if (capture_ != NULL) {
    flags |= kCapture;
  }
}

// 0x40cdc0
int CALLBACK HLWindow::NoWordBreak(LPSTR text, int start, int length,
                                   int code) {
  return 0;
}

// 0x40cdd0
HLWindow::HLWindow() {
  name_ = kName;
  app_ = NULL;
  title_ = name_ + strlen(name_);
  icon_ = 0;
  menu_ = 0;
  pgothic_font_ = NULL;
  gothic_font_ = NULL;
  fixed_font_ = NULL;
  message_field_ = NULL;
  buffer_ = NULL;
  message_height_ = 160;
  mouse_func_ = NULL;
  mouse_flags_ = 0;
}

// 0x40ce30
void HLWindow::Finalize() {
  SetMouseHandler(NULL, NULL);
  if (app_->window != NULL) {
    if (!(app_->flags & Application::kFullScreen)) {
      RECT rect;
      GetWindowRect(app_->window, &rect);
      Profile* profile = &app_->profile;
      profile->SetSection("DirectX");
      app_->profile.WriteInt("WndPosX", rect.left, "%d");
      app_->profile.WriteInt("WndPosY", rect.top, "%d");
    }
    SetWindowLong(app_->window, GWL_USERDATA, 0);
  }
  if (message_field_ != NULL) {
    DestroyWindow(message_field_);
    message_field_ = NULL;
  }
  if (buffer_ != NULL) {
    free(buffer_);
  }
  buffer_ = NULL;
  DeleteObject(fixed_font_);
  DeleteObject(gothic_font_);
  DeleteObject(pgothic_font_);
}

// 0x40cf30
LRESULT CALLBACK HLWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                   LPARAM lparam) {
  HLWindow* window;
  switch (msg) {
    case WM_CREATE:
      window = static_cast<HLWindow*>(
          reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
      SetWindowLong(hwnd, GWL_USERDATA, reinterpret_cast<LONG>(window));
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    default:
      window = reinterpret_cast<HLWindow*>(GetWindowLong(hwnd, GWL_USERDATA));
      break;
  }
  if (window == NULL) {
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  Application* app = window->app_;
  if (app->hook != NULL && !app->hook(hwnd, msg, wparam, lparam)) {
    return 0;
  }
  if (window->mouse_func_ != NULL && window->OnMouse(msg, wparam, lparam)) {
    return 0;
  }
  switch (msg) {
    case WM_PAINT:
      // The test of kSkipFrame always passes (a precedence slip).
      if ((app->graphics_flags & Application::kDirectDraw) &&
          (app->flags & Application::kDrawing) &&
          !(app->flags & Application::kFullScreen) &&
          (app->flags & Application::kPaused | Application::kSkipFrame)) {
        app->graphics->Blt();
      }
      break;
    case WM_ACTIVATE: {
      int minimized = HIWORD(wparam);
      if (LOWORD(wparam) == WA_INACTIVE) {
        if ((app->options & Application::kPauseInactive) || minimized) {
          app->Pause(1);
        }
        app->flags |= Application::kInactive;
      } else if (!minimized) {
        app->Pause(0);
        app->flags &= ~Application::kInactive;
      }
      break;
    }
    case WM_MOVE:
    case WM_SIZE:
      app->graphics->UpdateWindowRect(hwnd);
      break;
    case WM_USER + 0x1000:
      app->flags &= ~Application::kSkipFrame;
      break;
    case WM_SYSCOMMAND:
      switch (wparam) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
          return 0;
      }
      break;
    case WM_KEYDOWN:
      switch (wparam) {
        case VK_ESCAPE:
          if (app->options & Application::kSystemKeys) {
            app->Quit();
          }
          return 0;
        case VK_F12:
          if (app->options & Application::kSystemKeys) {
            app->RequestCapture();
          }
          return 0;
        case VK_F1:
          if (app->options & Application::kSyncKeys) {
            app->sync->SetMode(HLSync::kVSync);
          }
          return 0;
        case VK_F2:
          if (app->options & Application::kSyncKeys) {
            app->sync->SetMode(HLSync::kWait);
          }
          return 0;
        case VK_F3:
          if (app->options & Application::kSyncKeys) {
            app->sync->SetMode(HLSync::kNoWait);
          }
          return 0;
        case VK_F4:
          if (app->options & Application::kSyncKeys) {
            app->sync->SetMode(HLSync::kSkipDraw);
          }
          return 0;
        case VK_F5:
          app->flags &= ~Application::kSkipFrame;
          return 0;
      }
      break;
    case WM_CLOSE:
      app->Quit();
      return 0;
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

// 0x40d230
int HLWindow::Initialize(Application* app) {
  app_ = app;
  HICON icon;
  if (icon_ == 0) {
    icon = LoadIcon(NULL, IDI_APPLICATION);
  } else {
    icon = LoadIcon(app->instance, MAKEINTRESOURCE(icon_));
  }
  fixed_font_ = CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0,
                           0, 0, 0, "FixedSys");
  // "MS Gothic"
  gothic_font_ = CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET,
                            0, 0, 0, 0, "\202l\202r \203S\203V\203b\203N");
  // "MS PGothic"
  pgothic_font_ =
      CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0, 0,
                 "\202l\202r \202o\203S\203V\203b\203N");
  WNDCLASSEX window_class;
  window_class.cbSize = sizeof(window_class);
  window_class.lpszClassName = "hl_defwin";
  window_class.lpfnWndProc = WndProc;
  window_class.hInstance = app_->instance;
  window_class.style = CS_DBLCLKS;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hIconSm = icon;
  window_class.hIcon = icon;
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  if (app_->flags & Application::kFullScreen) {
    window_class.hbrBackground =
        static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    window_class.lpszMenuName = NULL;
  } else {
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    if (menu_ == 0) {
      window_class.lpszMenuName = NULL;
    } else {
      window_class.lpszMenuName = MAKEINTRESOURCE(menu_);
    }
  }
  if (!RegisterClassEx(&window_class)) {
    // "the window class cannot be registered"
    Error(
        "\263\262\335\304\336\263\270\327\275\202\252\223o\230^\202\305\202"
        "\253\202\334\202\271\202\361");
    return 0;
  }
  if (app_->flags & Application::kFullScreen) {
    app_->window = CreateWindowEx(WS_EX_APPWINDOW, "hl_defwin", title_,
                                  WS_POPUP | WS_VISIBLE, -200, -200, 100, 100,
                                  NULL, NULL, app_->instance, this);
    message_height_ = 0;
  } else {
    Profile* profile = &app_->profile;
    profile->SetSection("DirectX");
    int x = app_->profile.GetInt("WndPosX", CW_USEDEFAULT);
    int y = app_->profile.GetInt("WndPosY", CW_USEDEFAULT);
    if (!(app_->options & Application::kMessageField)) {
      message_height_ = 0;
    }
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = app_->width;
    rect.bottom = app_->height + message_height_;
    AdjustWindowRectEx(&rect, WS_CAPTION | WS_SYSMENU, menu_ != 0, 0);
    app_->window =
        CreateWindowEx(WS_EX_APPWINDOW, "hl_defwin", title_,
                       WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x,
                       y, rect.right - rect.left, rect.bottom - rect.top, NULL,
                       NULL, app_->instance, this);
  }
  if (app_->window == NULL) {
    // "the window cannot be created"
    Error(
        "\263\262\335\304\336\263\202\252\220\266\220\254\202\305\202\253\202"
        "\334\202\271\202\361");
    return 0;
  }
  if (message_height_ != 0) {
    message_field_ = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL |
            ES_READONLY | ES_WANTRETURN,
        0, app_->height, app_->width, message_height_, app_->window,
        reinterpret_cast<HMENU>(1), app_->instance, NULL);
    if (message_field_ == NULL) {
      // "the message field cannot be created"
      Error(
          "\322\257\276\260\274\336\314\250\260\331\304\336\202\252\220\266\220"
          "\254\202\305\202\253\202\334\202\271\202\361");
      return 0;
    }
    SendMessage(message_field_, WM_SETFONT,
                reinterpret_cast<WPARAM>(gothic_font_), TRUE);
    SendMessage(message_field_, EM_SETWORDBREAKPROC, 0,
                reinterpret_cast<LPARAM>(NoWordBreak));
    buffer_ = malloc(MAX_PATH);
    if (buffer_ == NULL) {
      Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
      return 0;
    }
  }
  return 1;
}
