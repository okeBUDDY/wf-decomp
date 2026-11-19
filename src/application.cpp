#include "application.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "graphics.h"
#include "hl_3d.h"
#include "hl_device.h"
#include "hl_input.h"
#include "hl_sync.h"
#include "hl_window.h"
#include "snd_sys.h"
#include "system.h"
#include "user_func.h"

// 0x406860
void Application::SetTimeStop(int stop) {
  if (stop) {
    flags |= kStopTime;
  } else {
    flags |= kStartTime;
  }
}

// 0x406890
int Application::AddSystem(System* system, bool owns) {
  if (system == NULL) {
    return 0;
  }
  if (system_count == kMaxSystems) {
    // "too many systems registered"
    Error("ｼｽﾃﾑの登録数が多すぎます");
    return 0;
  }
  systems[system_count] = system;
  owned[system_count] = owns;
  ++system_count;
  system->Log("Init");
  return system->Initialize(this);
}

// 0x406980
int Application::Initialize(const char* title, const char* name) {
  if (options & kLog) {
    strcpy(g_text_buffer, name);
    strcat(g_text_buffer, ".log");
    OpenLog(g_text_buffer, title);
    Log("AllInit");
  }
  srand(time(NULL));
  profile.Open(name);
  strncpy(this->title, title, 0x20);
  this->title[0x20] = '\0';
  strcat(this->title, "  ver 0.00");
  tasks.RemoveFrom(0);
  overlays.RemoveFrom(0);
  // The device and the window fail through one condition: the original
  // shares their `return 0` with the later failures.
  if (!AddSystem(device, true) ||
      (window_system->set_title(title), !AddSystem(window_system, true))) {
    return 0;
  }
  if (graphics_flags & kInput) {
    if (!AddSystem(input, true)) {
      return 0;
    }
  } else if (input != NULL) {
    delete input;
  }
  if (graphics_flags & kDirectDraw) {
    if (!AddSystem(graphics, true)) {
      return 0;
    }
    screen = graphics->back();
  } else if (graphics != NULL) {
    delete graphics;
  }
  if (graphics_flags & kDirect3D) {
    if (!AddSystem(d3d, true)) {
      return 0;
    }
  } else if (d3d != NULL) {
    delete d3d;
  }
  if (graphics_flags & kSound) {
    if (!AddSystem(sound, true)) {
      return 0;
    }
  } else if (sound != NULL) {
    delete sound;
  }
  return AddSystem(sync, true) ? 1 : 0;
}

// 0x406b80
void Application::Shutdown() {
  if (last_error()[0] != '\0') {
    MessageBox(window, last_error(), "ERROR", MB_ICONWARNING);
  }
  tasks.RemoveFrom(0);
  overlays.RemoveFrom(0);
  for (int i = system_count - 1; i >= 0; --i) {
    systems[i]->Finalize();
    systems[i]->Log("End");
    if (owned[i] && systems[i] != NULL) {
      delete systems[i];
    }
    systems[i] = NULL;
    owned[i] = false;
  }
  system_count = 0;
}

// 0x4067d0
void Application::DispatchMsg(MSG* msg) {
  TranslateMessage(msg);
  DispatchMessage(msg);
}

// 0x4067f0
void Application::Pause(int pause) {
  if (pause) {
    if (!(flags & kPaused)) {
      for (int i = 0; i < system_count; ++i) {
        systems[i]->Pause();
      }
      flags |= kPaused;
    }
  } else if (flags & kPaused) {
    flags = (flags & ~kPaused) | kResumed;
  }
}

// 0x406c20
int Application::Run() {
  MSG msg;
  Log("AllMove");
  flags |= kRunning;
  for (;;) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
      if (!GetMessage(&msg, NULL, 0, 0)) {
        return msg.wParam;
      }
      for (int i = dialog_count - 1; i >= 0; --i) {
        if (IsDialogMessage(dialogs[i], &msg)) {
          goto next;
        }
      }
      message_proc(&msg);
    next:;
    }
    if (flags & kResumed) {
      for (int i = system_count - 1; i >= 0; --i) {
        systems[i]->Resume();
      }
      flags &= ~kResumed;
    }
    if (flags & (kSkipFrame | kPaused)) {
      WaitMessage();
      continue;
    }
    if (flags & kTimeStopped) {
      tasks.RemoveUpTo(0);
      overlays.RemoveUpTo(0);
    } else {
      tasks.RemoveUpTo(2);
      user_func->Move();
      overlays.RemoveUpTo(2);
    }
    d3d->BeginScene();
    d3d->Clear();
    tasks.RunAll();
    user_func->Draw();
    overlays.RunAll();
    d3d->EndScene();
    sync->Present();
    if (flags & kReset) {
      flags |= kStartTime;
      tasks.RemoveFrom(2);
      overlays.RemoveFrom(2);
      user_func->Finish(reset_func);
      flags &= ~kReset;
    }
    UserFunc* func = user_func;
    if (!func->done_) {
      ++func->frames_;
    } else {
      func->Finalize();
      func->Log("End");
      if (func->next_ != NULL) {
        func->next_->Log("Init");
        if (func->next_->Initialize()) {
          func = func->next_;
          func->next_ = NULL;
          func->done_ = 0;
          func->frames_ = 0;
        } else {
          func->next_->Finalize();
          func = NULL;
        }
      } else {
        func = NULL;
      }
    }
    user_func = func;
    if (func == NULL) {
      flags &= ~kRunning;
      Shutdown();
      DestroyWindow(window);
    }
    if (!(flags & kTimeStopped)) {
      ++frame_count;
    }
    switch (flags & (kStopTime | kStartTime)) {
      case kStopTime:
        flags |= kTimeStopped;
        g_time_moves = 0;
        break;
      case kStartTime:
        flags &= ~kTimeStopped;
        g_time_moves = 1;
        break;
    }
    flags &= ~(kStopTime | kStartTime);
  }
}

// 0x406f10
Application::Application() {
  name_ = "HLApprication";
  for (int i = 0; i < kMaxSystems; ++i) {
    systems[i] = NULL;
    owned[i] = false;
  }
  system_count = 0;
  dialog_count = 0;
  static UserFunc s_idle;
  user_func = &s_idle;
  reset_func = NULL;
  hook = NULL;
  message_proc = DispatchMsg;
  device = new HLDevice;
  graphics = new Graphics;
  d3d = new HL3D;
  input = new HLInput;
  sound = new SoundSystem;
  window_system = new HLWindow;
  sync = new HLSync;
  screen = NULL;
  title[0] = '\0';
  frame_count = 0;
}

// 0x4071b0
void Application::Quit() {
  Log("AllEnd");
  if (flags & kRunning) {
    user_func->Finish(NULL);
    Pause(0);
    flags &= ~kSkipFrame;
  } else {
    Shutdown();
    exit(0);
  }
}
