#include "hl_input.h"

#include <stdio.h>
#include <string.h>

#include "application.h"
#include "hl_device.h"
#include "profile.h"

Profile* HLInpDev::profile_;            // 0x468b8c
unsigned char HLKeyboard::state_[256];  // 0x468b90
unsigned char HLKeyboard::scan_[256];   // 0x468c90
int HLKeyboard::trigger_keys_[kMaxTriggers] = {
    DIK_Z, DIK_X, DIK_S, DIK_D, DIK_A, DIK_W, DIK_E, DIK_F};  // 0x45d1d8

namespace {

// What EnumDeviceProc() gets.
struct EnumContext {
  IDirectInput7* dinput;
  HLDevSet* set;
};

}  // namespace

// 0x405a60
BOOL CALLBACK HLInput::EnumDeviceProc(LPCDIDEVICEINSTANCE instance,
                                      LPVOID context) {
  EnumContext* enum_context = static_cast<EnumContext*>(context);
  IDirectInput7* dinput = enum_context->dinput;
  HLDevSet* set = enum_context->set;
  IDirectInputDevice7* device;
  if (dinput->CreateDeviceEx(instance->guidInstance, IID_IDirectInputDevice7,
                             reinterpret_cast<void**>(&device),
                             NULL) == DI_OK) {
    device->Release();
    set->Add(&instance->guidInstance, instance->tszProductName, 0);
  }
  return DIENUM_CONTINUE;
}

// 0x405ab0
void HLJoystick::Poll() {
  DIJOYSTATE state;
  device_->Poll();
  HRESULT result = device_->GetDeviceState(sizeof(state), &state);
  switch (result) {
    case DI_OK: {
      if (state.lY < 0) {
        buttons_ |= kUp;
      }
      if (state.lY > 0) {
        buttons_ |= kDown;
      }
      if (state.lX < 0) {
        buttons_ |= kLeft;
      }
      if (state.lX > 0) {
        buttons_ |= kRight;
      }
      unsigned short bit = kTrigger;
      for (int i = 0; i < button_count_; ++i) {
        if (state.rgbButtons[trigger_map_[i]] & 0x80) {
          buttons_ |= bit;
        }
        bit <<= 1;
      }
      break;
    }
    case DIERR_INPUTLOST:
      device_->Acquire();
      device_->GetDeviceState(sizeof(state), &state);
      break;
  }
}

// 0x405b70
int HLJoystick::GetPressed() {
  DIJOYSTATE state;
  device_->Poll();
  ZeroMemory(&state, sizeof(state));
  HRESULT result = device_->GetDeviceState(sizeof(state), &state);
  switch (result) {
    case DI_OK:
      for (int i = 0; i < 32; ++i) {
        if (state.rgbButtons[i] & 0x80) {
          return i;
        }
      }
      break;
    case DIERR_INPUTLOST:
      device_->Acquire();
      device_->GetDeviceState(sizeof(state), &state);
      break;
  }
  return -1;
}

// 0x405c00
void HLKeyboard::Poll() {
  HRESULT result = device_->GetDeviceState(sizeof(state_), state_);
  switch (result) {
    case DI_OK: {
      if ((state_[DIK_NUMPAD2] | state_[DIK_DOWN]) & 0x80) {
        buttons_ |= kDown;
      }
      if ((state_[DIK_NUMPAD4] | state_[DIK_LEFT]) & 0x80) {
        buttons_ |= kLeft;
      }
      if ((state_[DIK_NUMPAD6] | state_[DIK_RIGHT]) & 0x80) {
        buttons_ |= kRight;
      }
      if ((state_[DIK_NUMPAD8] | state_[DIK_UP]) & 0x80) {
        buttons_ |= kUp;
      }
      if (state_[DIK_NUMPAD1] & 0x80) {
        buttons_ |= kDown | kLeft;
      }
      if (state_[DIK_NUMPAD3] & 0x80) {
        buttons_ |= kDown | kRight;
      }
      if (state_[DIK_NUMPAD7] & 0x80) {
        buttons_ |= kUp | kLeft;
      }
      if (state_[DIK_NUMPAD9] & 0x80) {
        buttons_ |= kUp | kRight;
      }
      unsigned short bit = kTrigger;
      for (int i = 0; i < button_count_; ++i) {
        if (state_[trigger_map_[i]] & 0x80) {
          buttons_ |= bit;
        }
        bit <<= 1;
      }
      break;
    }
    case DIERR_INPUTLOST:
      device_->Acquire();
      device_->GetDeviceState(sizeof(state_), state_);
      break;
  }
}

// 0x405d00
int HLKeyboard::GetPressed() {
  HRESULT result = device_->GetDeviceState(sizeof(scan_), scan_);
  switch (result) {
    case DI_OK:
      for (int i = 0; i < kMaxTriggers; ++i) {
        if (scan_[trigger_keys_[i]] & 0x80) {
          return trigger_keys_[i];
        }
      }
      return -1;
    case DIERR_INPUTLOST:
      device_->Acquire();
      device_->GetDeviceState(sizeof(scan_), scan_);
      return -1;
  }
  return -1;
}

// 0x405d80
void HLKeyboard::GetName(int code, char* out) {
  strcpy(out, "  ");
  switch (code) {
    case DIK_Z:
      out[0] = 'Z';
      break;
    case DIK_X:
      out[0] = 'X';
      break;
    case DIK_S:
      out[0] = 'S';
      break;
    case DIK_D:
      out[0] = 'D';
      break;
    case DIK_A:
      out[0] = 'A';
      break;
    case DIK_W:
      out[0] = 'W';
      break;
    case DIK_E:
      out[0] = 'E';
      break;
    case DIK_F:
      out[0] = 'F';
      break;
  }
}

// 0x405e30
int InputTask::Initialize() { return input_ != NULL; }

// 0x405e40
HLInput::HLInput() {
  dinput_ = NULL;
  time_stop_mask_ = 0;
  app_ = NULL;
  task_ = NULL;
  name_ = "HLInput";
  pad_count_ = 1;
  max_buttons_ = 8;
  for (int i = 0; i < kMaxPads; ++i) {
    guids_[i] = NULL;
    pads_[i] = NULL;
    devs_[i] = NULL;
  }
}

// 0x405ea0
int HLInput::EnumDevices(Application* app, HLDevSet* set) {
  EnumContext context;
  context.dinput = NULL;
  context.set = set;
  if (DirectInputCreateEx(app->instance, DIRECTINPUT_VERSION, IID_IDirectInput7,
                          reinterpret_cast<void**>(&context.dinput),
                          NULL) != DI_OK) {
    return 0;
  }
  context.dinput->EnumDevices(DIDEVTYPE_JOYSTICK, EnumDeviceProc, &context,
                              DIEDFL_ATTACHEDONLY);
  context.dinput->Release();
  set->Add(NULL, "Keyboard", 1);
  return set->count() != 0;
}

// 0x405f30
// TODO: nonmatching: the loop index and the devs_ offset swap ebx/ebp and
// the returns are not merged.
int InputTask::IsProtected() {
  if (!(input_->app_->flags & Application::kInactive)) {
    for (int i = 0; i < input_->pad_count_; ++i) {
      if (input_->devs_[i] != NULL) {
        HLInpDev* dev = input_->devs_[i];
        dev->buttons_ = 0;
        dev->Poll();
        dev->pressed_ = dev->buttons_ & ~dev->previous_;
        dev->previous_ = dev->buttons_;
        if (dev->pressed_ & input_->time_stop_mask_) {
          if (input_->app_->flags & Application::kTimeStopped) {
            input_->app_->SetTimeStop(0);
          } else {
            input_->app_->SetTimeStop(1);
          }
        }
      }
    }
  }
  return 1;
}

// 0x405fc0
void HLJoystick::GetName(int code, char* out) { sprintf(out, "%-2d", code); }

// 0x405fe0
void HLInpDev::LoadTriggerMap() {
  DIDEVICEINSTANCE instance;
  if (profile_ == NULL) {
    return;
  }
  ZeroMemory(&instance, sizeof(instance));
  instance.dwSize = sizeof(instance);
  if (device_->GetDeviceInfo(&instance) != DI_OK) {
    return;
  }
  profile_->SetSection("TriggerMap");
  profile_->ReadString(instance.tszProductName, g_text_buffer,
                       sizeof(g_text_buffer), "");
  if (g_text_buffer[0] != '\0') {
    int map[kMaxTriggers];
    sscanf(g_text_buffer, "%d %d %d %d %d %d %d %d", &map[0], &map[1], &map[2],
           &map[3], &map[4], &map[5], &map[6], &map[7]);
    for (int i = 0; i < kMaxTriggers; ++i) {
      trigger_map_[i] = map[i];
    }
  }
  unsigned short buttons = kMaxTriggers;
  if (instance.dwDevType & DIDEVTYPE_JOYSTICK) {
    DIDEVCAPS caps;
    ZeroMemory(&caps, sizeof(caps));
    caps.dwSize = sizeof(caps);
    if (device_->GetCapabilities(&caps) == DI_OK) {
      buttons = caps.dwButtons;
    }
  }
  if (buttons < button_count_) {
    button_count_ = buttons;
  }
  // "%d-button pad (%s) usable: checking %d buttons: TrigMap=%s"
  Log("%d\316\336\300\335\312\337\257\304\336(%s)\202\360\227\230\227p\211\302"
      ":%d\316\336\300\335\202\360\222\262\215\270:TrigMap=%s",
      buttons, instance.tszProductName, button_count_, g_text_buffer);
}

// 0x406170
void HLInpDev::SaveTriggerMap() {
  DIDEVICEINSTANCE instance;
  if (profile_ == NULL) {
    return;
  }
  sprintf(g_text_buffer, "%d %d %d %d %d %d %d %d", trigger_map_[0],
          trigger_map_[1], trigger_map_[2], trigger_map_[3], trigger_map_[4],
          trigger_map_[5], trigger_map_[6], trigger_map_[7]);
  ZeroMemory(&instance, sizeof(instance));
  instance.dwSize = sizeof(instance);
  if (device_->GetDeviceInfo(&instance) != DI_OK) {
    return;
  }
  profile_->SetSection("TriggerMap");
  profile_->WriteString(instance.tszProductName, g_text_buffer);
}

// 0x406260
int HLJoystick::Open(HLInput* input, const GUID* guid) {
  if (input->dinput_->CreateDeviceEx(*guid, IID_IDirectInputDevice7,
                                     reinterpret_cast<void**>(&device_),
                                     NULL) != DI_OK) {
    // "Pad device unavailable"
    Error(
        "\312\337\257\304\336\303\336\312\336\262\275\202\252\227\230\227p"
        "\202\305\202\253\202\334\202\271\202\361");
    return 0;
  }
  if (device_->SetDataFormat(&c_dfDIJoystick) != DI_OK) {
    // "Cannot set the pad format"
    Error(
        "\312\337\257\304\336\214`\216\256\202\360\220\335\222\350\202\305"
        "\202\253\202\334\202\271\202\361");
    return 0;
  }
  if (device_->SetCooperativeLevel(input->app_->window,
                                   DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) !=
      DI_OK) {
    // "Cannot set the pad cooperative level"
    Error(
        "\312\337\257\304\336\202\314\213\246\222\262\332\315\336\331\202\360"
        "\220\335\222\350\202\305\202\253\202\334\202\271\202\361");
    return 0;
  }
  DIPROPRANGE range;
  DIPROPDWORD dead_zone;
  range.diph.dwHeaderSize = sizeof(range.diph);
  dead_zone.diph.dwHeaderSize = sizeof(dead_zone.diph);
  range.diph.dwHow = DIPH_BYOFFSET;
  dead_zone.diph.dwHow = DIPH_BYOFFSET;
  range.diph.dwObj = DIJOFS_X;
  dead_zone.diph.dwObj = DIJOFS_X;
  dead_zone.diph.dwSize = sizeof(dead_zone);
  range.diph.dwSize = sizeof(range);
  dead_zone.dwData = 5000;
  range.lMin = -1000;
  range.lMax = 1000;
  HRESULT result = device_->SetProperty(DIPROP_RANGE, &range.diph);
  result |= device_->SetProperty(DIPROP_DEADZONE, &dead_zone.diph);
  range.diph.dwObj = DIJOFS_Y;
  dead_zone.diph.dwObj = DIJOFS_Y;
  result |= device_->SetProperty(DIPROP_RANGE, &range.diph);
  result |= device_->SetProperty(DIPROP_DEADZONE, &dead_zone.diph);
  if (result != DI_OK) {
    // "Cannot adjust the pad"
    Error(
        "\312\337\257\304\336\202\314\222\262\220\256\202\252\202\305\202\253"
        "\202\334\202\271\202\361");
    return 0;
  }
  for (int i = 0; i < kMaxTriggers; ++i) {
    trigger_map_[i] = i;
  }
  button_count_ = input->max_buttons_;
  LoadTriggerMap();
  if (device_->Acquire() != DI_OK) {
    // "Cannot start using the pad"
    Error(
        "\312\337\257\304\336\202\252\227\230\227p\212J\216n\202\305\202\253"
        "\202\334\202\271\202\361");
    return 0;
  }
  return 1;
}

// 0x406420
int HLKeyboard::Open(HLInput* input, const GUID* guid) {
  if (input->dinput_->CreateDeviceEx(GUID_SysKeyboard, IID_IDirectInputDevice7,
                                     reinterpret_cast<void**>(&device_),
                                     NULL) != DI_OK) {
    // "Keyboard device unavailable"
    Error(
        "\267\260\316\336\260\304\336\303\336\312\336\262\275\202\252\227\230"
        "\227p\202\305\202\253\202\334\202\271\202\361");
    return 0;
  }
  if (device_->SetDataFormat(&c_dfDIKeyboard) != DI_OK) {
    // "Cannot set the keyboard format"
    Error(
        "\267\260\316\336\260\304\336\214`\216\256\202\360\220\335\222\350"
        "\202\305\202\253\202\334\202\271\202\361");
    return 0;
  }
  if (device_->SetCooperativeLevel(input->app_->window,
                                   DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) !=
      DI_OK) {
    // "Cannot set the keyboard cooperative level"
    Error(
        "\267\260\316\336\260\304\336\202\314\213\246\222\262\332\315\336\331"
        "\202\360\220\335\222\350\202\305\202\253\202\334\202\271\202\361");
    return 0;
  }
  trigger_map_[0] = DIK_Z;
  trigger_map_[1] = DIK_X;
  trigger_map_[2] = DIK_S;
  trigger_map_[3] = DIK_D;
  trigger_map_[4] = DIK_A;
  trigger_map_[5] = DIK_W;
  trigger_map_[6] = DIK_E;
  trigger_map_[7] = DIK_F;
  button_count_ = input->max_buttons_;
  LoadTriggerMap();
  if (device_->Acquire() != DI_OK) {
    // "Cannot start using the keyboard"
    Error(
        "\267\260\316\336\260\304\336\202\252\227\230\227p\212J\216n\202\305"
        "\202\253\202\334\202\271\202\361");
    return 0;
  }
  return 1;
}

// 0x406530
int HLInput::Initialize(Application* app) {
  app_ = app;
  if (DirectInputCreateEx(app->instance, DIRECTINPUT_VERSION, IID_IDirectInput7,
                          reinterpret_cast<void**>(&dinput_), NULL) != DI_OK) {
    // "Cannot get DirectInput7"
    Error(
        "DirectInput7\202\252\216\346\223\276\202\305\202\253\202\334\202\271"
        "\202\361");
    return 0;
  }
  HLInpDev::profile_ = &app_->profile;
  for (int i = 0; i < pad_count_; ++i) {
    int j;
    for (j = 0; j < i; ++j) {
      if (guids_[i] == guids_[j]) {
        break;
      }
    }
    if (i != j) {
      devs_[i] = NULL;
      pads_[i] = devs_[j];
    } else {
      if (guids_[i] == NULL) {
        devs_[i] = new HLKeyboard;
      } else {
        devs_[i] = new HLJoystick;
      }
      if (devs_[i] == NULL) {
        goto no_memory;
      }
      if (!devs_[i]->Open(this, guids_[i])) {
        return 0;
      }
      pads_[i] = devs_[i];
    }
  }
  task_ = new InputTask;
  if (task_ == NULL) {
  no_memory:
    // "Out of memory"
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  task_->input_ = this;
  app_->tasks.AddBack(task_, 0);
  return 1;
}

// 0x406750
void HLInput::Finalize() {
  for (int i = 0; i < pad_count_; ++i) {
    HLInpDev* dev = devs_[i];
    if (dev != NULL) {
      if (dev->device_ != NULL) {
        dev->SaveTriggerMap();
        dev->device_->Unacquire();
        dev->device_->Release();
        dev->device_ = NULL;
      }
      if (devs_[i] != NULL) {
        delete devs_[i];
      }
    }
    devs_[i] = NULL;
    pads_[i] = NULL;
  }
  if (dinput_ != NULL) {
    dinput_->Release();
  }
  dinput_ = NULL;
}
