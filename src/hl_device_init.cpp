// HLDevice::Initialize, the start-up device dialog (its own object file in
// the original link).
#include "application.h"
#include "graphics.h"
#include "hl_device.h"
#include "hl_input.h"
#include "snd_sys.h"

// 0x404f90
int HLDevice::Initialize(Application* app) {
  app_ = app;
  if (app->graphics_flags & AppSettings::kInput) {
    pad_count_ = app->input->pad_count();
    if (pad_count_ > HLInput::kMaxPads) {
      pad_count_ = HLInput::kMaxPads;
    }
  }
  pads_ = new HLDevSet[pad_count_];
  if (!graphics_.Alloc() || !sound_.Alloc() || pads_ == NULL) {
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  for (int i = 0; i < pad_count_; ++i) {
    if (!pads_[i].Alloc()) {
      Error("ﾒﾓﾘが足りません");
      return 0;
    }
  }
  if (app_->graphics_flags & AppSettings::kDirectDraw) {
    if (!app_->graphics->EnumDevices(&graphics_)) {
      Error("DirectDraw7が利用できません");
      return 0;
    }
  }
  if (app_->graphics_flags & AppSettings::kInput) {
    app_->input->EnumDevices(app_, pads_);
    for (int i = 1; i < pad_count_; ++i) {
      pads_[i].Assign(pads_[0]);
    }
  }
  if (app_->graphics_flags & AppSettings::kSound) {
    app_->sound->EnumDevices(&sound_);
  }
  LoadSettings();
  for (;;) {
    if ((app_->options & AppSettings::kDeviceDialog) &&
        !(flags_ & kFlagRemember)) {
      if (!DialogBoxParam(app_->instance, MAKEINTRESOURCE(101), app_->window,
                          DialogProc, reinterpret_cast<LPARAM>(this))) {
        Log("ﾕｰｻﾞｰによるｷｬﾝｾﾙ");
        return 0;
      }
      break;
    }
    if (CheckDisplay(NULL)) {
      break;
    }
    // Asks again (in the dialog, when there is one).
    if (!(app_->options & AppSettings::kDeviceDialog)) {
      return 0;
    }
    flags_ &= ~kFlagRemember;
  }
  SaveSettings();
  graphics_.Clear();
  sound_.Clear();
  for (int i = 0; i < pad_count_; ++i) {
    pads_[i].Clear();
  }
  return 1;
}
