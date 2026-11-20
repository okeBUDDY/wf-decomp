#include "graphics.h"

#include <stdlib.h>

#include "application.h"
#include "hl_device.h"

// 0x405270
BOOL WINAPI Graphics::EnumDeviceProc(GUID* guid, LPSTR description, LPSTR name,
                                     LPVOID context, HMONITOR monitor) {
  IDirectDraw7* ddraw;
  if (DirectDrawCreateEx(guid, reinterpret_cast<void**>(&ddraw),
                         IID_IDirectDraw7, NULL) == DD_OK) {
    DDCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    caps.dwSize = sizeof(caps);
    if (ddraw->GetCaps(&caps, NULL) == DD_OK && (caps.dwCaps & DDCAPS_3D) &&
        (caps.ddsCaps.dwCaps & DDSCAPS_TEXTURE)) {
      static_cast<HLDevSet*>(context)->Add(guid, description, 0);
    }
    ddraw->Release();
  }
  return DDENUMRET_OK;
}

// 0x405300
void Graphics::UpdateWindowRect(HWND window) {
  if (app_ != NULL) {
    window_rect_.top = 0;
    window_rect_.left = 0;
    ClientToScreen(window, reinterpret_cast<POINT*>(&window_rect_));
    window_rect_.right = window_rect_.left + app_->width;
    window_rect_.bottom = window_rect_.top + app_->height;
  }
}

// 0x405350
void Graphics::ClipCursor(int clip) {
  if (app_ != NULL && (app_->flags & Application::kFullScreen)) {
    if (clip) {
      ::ClipCursor(&window_rect_);
      SetCursorPos((window_rect_.left + window_rect_.right) / 2,
                   (window_rect_.top + window_rect_.bottom) / 2);
      if ((app_->options & Application::kHideCursor) &&
          !(flags_ & kCursorHidden)) {
        ShowCursor(FALSE);
        flags_ |= kCursorHidden;
      }
    } else {
      ::ClipCursor(NULL);
      if (flags_ & kCursorHidden) {
        ShowCursor(TRUE);
        flags_ &= ~kCursorHidden;
      }
    }
  }
}

// 0x4053e0
Graphics::Graphics() {
  ddraw_ = NULL;
  device_ = NULL;
  app_ = NULL;
  flags_ = 0;
  clipper_ = NULL;
  surfaces_ = NULL;
  name_ = "HLDraw";
}

// 0x405480
int Graphics::Initialize(Application* app) {
  app_ = app;
  HLSurface::graphics_ = this;
  surfaces_ =
      static_cast<HLSurface**>(malloc(kMaxSurfaces * sizeof(HLSurface*)));
  if (surfaces_ == NULL) {
    // "Out of memory"
    Error("メモリが足りません");
    return 0;
  }
  for (int i = 0; i < kMaxSurfaces; ++i) {
    surfaces_[i] = NULL;
  }
  UpdateWindowRect(app_->window);
  DDSURFACEDESC2 desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwSize = sizeof(desc);
  desc.dwFlags = DDSD_CAPS;
  desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
  if (app_->flags & Application::kFullScreen) {
    // "Starting full screen: %dx%d %dbit"
    Log("ﾌﾙｽｸﾘｰﾝﾓｰﾄﾞで起動:%dx%d %dbit", app_->width, app_->height,
        app_->depth);
    if (DirectDrawCreateEx(device_, reinterpret_cast<void**>(&ddraw_),
                           IID_IDirectDraw7, NULL) != DD_OK) {
      goto no_ddraw;
    }
    if (ddraw_->SetCooperativeLevel(
            app_->window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) != DD_OK) {
      goto no_cooperative_level;
    }
    if (ddraw_->SetDisplayMode(app_->width, app_->height, app_->depth, 0, 0) !=
        DD_OK) {
      // "%dx%d %dbit mode is unavailable"
      Error("%dx%d %dbitﾓｰﾄﾞが利用できません", app_->width, app_->height,
            app_->depth);
      return 0;
    }
    flags_ |= kDisplayModeSet;
    desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
    desc.dwBackBufferCount = 1;
    desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
  } else {
    // "Starting in a window: %dx%d %dbit"
    Log("ｳｲﾝﾄﾞｳﾓｰﾄﾞで起動:%dx%d %dbit", app_->width, app_->height, app_->depth);
    if (DirectDrawCreateEx(NULL, reinterpret_cast<void**>(&ddraw_),
                           IID_IDirectDraw7, NULL) != DD_OK) {
    no_ddraw:
      // "Cannot get DirectDraw7"
      Error("DirectDraw7が取得できません");
      return 0;
    }
    if (ddraw_->SetCooperativeLevel(app_->window, DDSCL_NORMAL) != DD_OK) {
    no_cooperative_level:
      // "Cannot set the DirectDraw cooperative level"
      Error("DirectDraw協調ﾚﾍﾞﾙが設定できません");
      return 0;
    }
  }
  if (ddraw_->CreateSurface(&desc, &primary_.surface_, NULL) != DD_OK) {
    // "Cannot create the primary surface"
    Error("ﾌﾟﾗｲﾏﾘｻｰﾌｪｽが生成できません");
    return 0;
  }
  primary_.Attach(HLSurface::kPrimary);
  if (app_->flags & Application::kFullScreen) {
    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(caps));
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    if (primary_.surface_->GetAttachedSurface(&caps, &back_.surface_) !=
        DD_OK) {
      // "Cannot create the back surface"
      Error("ﾊﾞｯｸｻｰﾌｪｽが生成できません");
      return 0;
    }
    back_.Attach(HLSurface::kBackBuffer);
  } else {
    if (!back_.Create(app_->width, app_->height,
                      HLSurface::kOffscreen | HLSurface::kRenderTarget)) {
      return 0;
    }
    app_->options |= Application::kClipper;
  }
  if (app_->options & Application::kClipper) {
    if (ddraw_->CreateClipper(0, &clipper_, NULL) != DD_OK) {
      // "Cannot create the clipper"
      Error("ｸﾘｯﾊﾟが生成できません");
      return 0;
    }
    if (clipper_->SetHWnd(0, app_->window) != DD_OK) {
      // "Cannot use the clipper"
      Error("ｸﾘｯﾊﾟを利用できません");
      return 0;
    }
    if (primary_.surface_->SetClipper(clipper_) != DD_OK) {
      // "Cannot attach the clipper to the surface"
      Error("ｸﾘｯﾊﾟをｻｰﾌｪｽにｱﾀｯﾁできません");
      return 0;
    }
  }
  ClipCursor(1);
  return 1;
}

// 0x405810
void Graphics::Finalize() {
  if (ddraw_ == NULL) {
    return;
  }
  if (surfaces_ != NULL) {
    for (int i = 0; i < kMaxSurfaces; ++i) {
      if (surfaces_[i] != NULL) {
        surfaces_[i]->Release();
      }
    }
    free(surfaces_);
    surfaces_ = NULL;
  }
  if (clipper_ != NULL) {
    primary_.surface_->SetClipper(NULL);
    clipper_->Release();
    clipper_ = NULL;
  }
  back_.Release();
  primary_.Release();
  ClipCursor(0);
  if (flags_ & kDisplayModeSet) {
    ddraw_->RestoreDisplayMode();
    flags_ &= ~kDisplayModeSet;
  }
  ddraw_->Release();
  ddraw_ = NULL;
}

// 0x4058e0
void Graphics::Pause() { ClipCursor(0); }

// 0x405910
void Graphics::Resume() {
  ClipCursor(1);
  if (surfaces_ != NULL) {
    for (int i = 0; i < kMaxSurfaces; ++i) {
      if (surfaces_[i] != NULL) {
        surfaces_[i]->Restore();
      }
    }
  }
  primary_.Restore();
  back_.Restore();
}

// 0x405a30
int Graphics::EnumDevices(HLDevSet* set) {
  if (DirectDrawEnumerateEx(EnumDeviceProc, set,
                            DDENUM_ATTACHEDSECONDARYDEVICES) != DD_OK) {
    return 0;
  }
  return set->count() != 0;
}
