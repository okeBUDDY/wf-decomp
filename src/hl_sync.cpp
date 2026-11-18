#include "hl_sync.h"

#include <mmsystem.h>

#include "application.h"
#include "graphics.h"

static char kName[] = "HLSync";  // 0x45d324

// 0x40c6b0
void HLSync::Finalize() {
  if (timer_ != 0) {
    timeKillEvent(timer_);
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof(caps));
    timeEndPeriod(caps.wPeriodMin);
  }
}

// 0x40c6e0
void CALLBACK HLSync::TimeProc(UINT id, UINT msg, DWORD_PTR user, DWORD_PTR dw1,
                               DWORD_PTR dw2) {
  HLSync* sync = reinterpret_cast<HLSync*>(user);
  ++sync->ticks_;
  sync->ticked_ = 1;
  if (sync->ticks_ == sync->fps_) {
    sync->measured_fps_ = sync->frames_;
    sync->frames_ = 0;
    sync->ticks_ = 0;
  }
}

// 0x40c710
void HLSync::Wait(HLSync* sync) {
  while (!sync->ticked_) {
  }
  sync->ticked_ = 0;
}

// 0x40c720
void HLSync::SkipFrame(HLSync* sync) {
  sync->app_->flags |= Application::kSkipFrame;
}

// 0x40c750
HLSync::HLSync() {
  app_ = NULL;
  timer_ = 0;
  name_ = kName;
  mode_ = kWait;
  fps_ = 60;
}

// 0x40c780
void HLSync::Flip(HLSync* sync) { sync->app_->graphics->Flip(); }

// 0x40c7a0
void HLSync::WaitBltFast(HLSync* sync) {
  Wait(sync);
  sync->app_->graphics->BltFast();
}

// 0x40c7d0
void HLSync::WaitBlt(HLSync* sync) {
  Wait(sync);
  sync->app_->graphics->Blt();
}

// 0x40c810
void HLSync::BltFast(HLSync* sync) { sync->app_->graphics->BltFast(); }

// 0x40c840
void HLSync::Blt(HLSync* sync) { sync->app_->graphics->Blt(); }

// 0x40c870
void HLSync::BltFastSkipFrame(HLSync* sync) {
  sync->app_->graphics->BltFast();
  sync->app_->flags |= Application::kSkipFrame;
}

// 0x40c8a0
void HLSync::BltSkipFrame(HLSync* sync) {
  sync->app_->graphics->Blt();
  sync->app_->flags |= Application::kSkipFrame;
}

// 0x40c8e0
void HLSync::SetMode(unsigned char mode) {
  Application* app = app_;
  if (app->graphics_flags & Application::kDirectDraw) {
    if (app->flags & Application::kFullScreen) {
      switch (mode) {
        case kVSync:
          present_ = Flip;
          break;
        case kWait:
          present_ = WaitBltFast;
          break;
        case kNoWait:
          present_ = BltFast;
          break;
        case kSkipDraw:
          present_ = BltFastSkipFrame;
          break;
      }
    } else {
      switch (mode) {
        case kVSync:
        case kWait:
          present_ = WaitBlt;
          break;
        case kNoWait:
          present_ = Blt;
          break;
        case kSkipDraw:
          present_ = BltSkipFrame;
          break;
      }
    }
  } else {
    switch (mode) {
      case kVSync:
      case kWait:
        present_ = Wait;
        break;
      case kNoWait:
        present_ = NoWait;
        break;
      case kSkipDraw:
        present_ = SkipFrame;
        break;
    }
  }
  measured_fps_ = 0;
  ticks_ = 0;
  frames_ = 0;
  ticked_ = 0;
  mode_ = mode;
  if (mode == kSkipDraw) {
    app->flags |= Application::kSkipFrame;
  } else {
    app->flags &= ~Application::kSkipFrame;
  }
}

// 0x40c9e0
int HLSync::Initialize(Application* app) {
  app_ = app;
  TIMECAPS caps;
  timeGetDevCaps(&caps, sizeof(caps));
  UINT delay = (10000 / fps_ + 5) / 10;
  if (delay < caps.wPeriodMin) {
    delay = caps.wPeriodMin;
  }
  timeBeginPeriod(caps.wPeriodMin);
  timer_ = timeSetEvent(delay, caps.wPeriodMin, TimeProc,
                        reinterpret_cast<DWORD_PTR>(this), TIME_PERIODIC);
  if (timer_ == 0) {
    // "the high resolution timer cannot be initialized"
    Error("高精度ﾀｲﾏｰが初期化できません");
    return 0;
  }
  SetMode(mode_);
  // "%dfps(1f=%dmsec): high resolution timer started"
  Log("%dfps(1f=%dmsec)で高精度ﾀｲﾏｰ起動", fps_, delay);
  return 1;
}
