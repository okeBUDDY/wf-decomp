// HLDevice and HLDevSet: the device choices of the start-up dialog.
#include "hl_device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "graphics.h"
#include "hl_input.h"
#include "profile.h"

// What an empty set shows and stores.
char kNoDevice[] = "No device";  // 0x45d1ac

// 0x4040c0
int HLDevSet::Select(const char* name) {
  if (count_ == 0 && strcmp(name, kNoDevice) == 0) {
    return 1;
  }
  for (int i = 0; i < count_; ++i) {
    if (strncmp(entries_[i].name, name, sizeof(entries_[i].name) - 1) == 0) {
      selected_ = i;
      guid_ = entries_[selected_].guid;
      kind_ = entries_[selected_].kind;
      return 1;
    }
  }
  return 0;
}

// 0x404180
void HLDevSet::Select(unsigned char index) {
  if (count_ != 0) {
    selected_ = index;
    if (selected_ > count_ - 1) {
      selected_ = count_ - 1;
    }
    guid_ = entries_[selected_].guid;
    kind_ = entries_[selected_].kind;
  }
}

// 0x4041e0
void HLDevSet::Fill(HWND combo, int enabled) {
  if (!enabled) {
    SendMessageA(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>("使用しません"));
    EnableWindow(combo, FALSE);
  } else if (count_ == 0) {
    SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kNoDevice));
  } else {
    for (int i = 0; i < count_; ++i) {
      SendMessageA(combo, CB_ADDSTRING, 0,
                   reinterpret_cast<LPARAM>(entries_[i].name));
    }
  }
  SendMessageA(combo, CB_SETCURSEL, selected_, 0);
}

// 0x404280
void HLDevSet::ReadSelection(HWND combo) {
  if (count_ == 0) {
    selected_ = 0;
    kind_ = kDefault;
    return;
  }
  selected_ =
      static_cast<unsigned char>(SendMessageA(combo, CB_GETCURSEL, 0, 0));
  guid_ = entries_[selected_].guid;
  kind_ = entries_[selected_].kind;
}

// 0x4042f0
void HLDevSet::CopyName(char* out, const char* name) {
  *out = '\0';
  while (*name == ' ') {
    if (*name == '\0') {
      return;
    }
    ++name;
  }
  strncpy(out, name, 63);
  out[63] = '\0';
  for (char* end = out + strlen(out) - 1; *end == ' '; --end) {
    *end = '\0';
  }
}

// 0x404350
void HLDevice::Finalize() {
  graphics_.Clear();
  sound_.Clear();
  if (pads_ != NULL) {
    for (int i = 0; i < pad_count_; ++i) {
      pads_[i].Clear();
    }
    delete[] pads_;
    pads_ = NULL;
  }
}

// 0x4043f0
int HLDevice::CheckDisplay(HWND window) {
  if (app_->flags & AppSettings::kFullScreen) {
    return 1;
  }
  HDC dc = GetDC(GetDesktopWindow());
  short depth = GetDeviceCaps(dc, PLANES) * GetDeviceCaps(dc, BITSPIXEL);
  ReleaseDC(GetDesktopWindow(), dc);
  if (depth == 8) {
    Warning(window, "256色ｳｲﾝﾄﾞｳﾓｰﾄﾞでは起動できません");
    return 0;
  }
  if (depth != app_->depth) {
    app_->depth = depth;
  }
  return 1;
}

// 0x404480
void HLDevice::InitDialog(HWND dialog) {
  graphics_.Fill(GetDlgItem(dialog, 0x3eb),
                 app_->graphics_flags & AppSettings::kDirectDraw);
  sound_.Fill(GetDlgItem(dialog, 0x3ed),
              app_->graphics_flags & AppSettings::kSound);
  pads_[0].Fill(GetDlgItem(dialog, 0x3ec),
                app_->graphics_flags & AppSettings::kInput);
  HWND pads = GetDlgItem(dialog, 0x3ee);
  for (int i = 0; i < pad_count_; ++i) {
    sprintf(g_text_buffer, "%d", i + 1);
    SendMessageA(pads, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(g_text_buffer));
  }
  SendMessageA(pads, CB_SETCURSEL, 0, 0);
  if (!(app_->graphics_flags & AppSettings::kInput)) {
    EnableWindow(pads, FALSE);
  }
  pad_ = 0;
  if (app_->flags & AppSettings::kFullScreen) {
    CheckRadioButton(dialog, 0x3e9, 0x3ea, 0x3ea);
    if (!(app_->options & AppSettings::kScreenChoice)) {
      EnableWindow(GetDlgItem(dialog, 0x3e9), FALSE);
    }
  } else {
    CheckRadioButton(dialog, 0x3e9, 0x3ea, 0x3e9);
    if (!(app_->options & AppSettings::kScreenChoice)) {
      EnableWindow(GetDlgItem(dialog, 0x3ea), FALSE);
    }
  }
  if (flags_ & kFlagRemember) {
    CheckDlgButton(dialog, 0x3ef, BST_CHECKED);
  }
}

// 0x4045e0
void HLDevice::ReadDialog(HWND dialog) {
  graphics_.ReadSelection(GetDlgItem(dialog, 0x3eb));
  sound_.ReadSelection(GetDlgItem(dialog, 0x3ed));
  if (IsDlgButtonChecked(dialog, 0x3ea) == BST_CHECKED) {
    app_->flags |= AppSettings::kFullScreen;
  } else {
    app_->flags &= ~AppSettings::kFullScreen;
  }
  if (IsDlgButtonChecked(dialog, 0x3ef) == BST_CHECKED) {
    flags_ |= kFlagRemember;
  } else {
    flags_ &= ~kFlagRemember;
  }
}

// 0x404730
void HLDevSet::Add(const GUID* guid, const char* name, int named) {
  if (count_ == kMaxEntries) {
    return;
  }
  if (named) {
    strncpy(entries_[count_].name, name, sizeof(entries_[count_].name) - 1);
    entries_[count_].name[sizeof(entries_[count_].name) - 1] = '\0';
    entries_[count_].kind = kNamed;
    ++count_;
    return;
  }
  if (guid == NULL) {
    strcpy(entries_[count_].name, "Default device");
    entries_[count_].kind = kDefault;
    ++count_;
    return;
  }
  CopyName(entries_[count_].name, name);
  entries_[count_].guid = *guid;
  entries_[count_].kind = kGuid;
  ++count_;
}

// 0x404840
HLDevice::HLDevice() {
  name_ = "HLDevice";
  app_ = NULL;
  pads_ = NULL;
  pad_ = 0;
  flags_ = 0;
  pad_count_ = 1;
}

// 0x404910
HLDevice::~HLDevice() {}

// 0x404e40
// TODO: nonmatching: the original loads the selection into eax, not ecx,
// for the last SendMessageA; everything else matches.
INT_PTR CALLBACK HLDevice::DialogProc(HWND dialog, UINT message, WPARAM wparam,
                                      LPARAM lparam) {
  if (message == WM_INITDIALOG) {
    HLDevice* device = reinterpret_cast<HLDevice*>(lparam);
    SetWindowLongA(dialog, GWL_USERDATA, lparam);
    device->InitDialog(dialog);
    return FALSE;
  }
  HLDevice* device =
      reinterpret_cast<HLDevice*>(GetWindowLongA(dialog, GWL_USERDATA));
  switch (message) {
    case WM_CLOSE:
      EndDialog(dialog, 0);
      return TRUE;
    case WM_COMMAND:
      switch (LOWORD(wparam)) {
        case IDOK:
          device->ReadDialog(dialog);
          if (device->CheckDisplay(dialog)) {
            EndDialog(dialog, 1);
          }
          return TRUE;
        case IDCANCEL:
          EndDialog(dialog, 0);
          return TRUE;
        case 0x3ec:
          if (HIWORD(wparam) == CBN_SELCHANGE) {
            device->pads_[device->pad_].Select(
                static_cast<unsigned char>(SendMessageA(
                    reinterpret_cast<HWND>(lparam), CB_GETCURSEL, 0, 0)));
            return TRUE;
          }
          // Falls through (to no effect).
        case 0x3ee:
          if (HIWORD(wparam) == CBN_SELCHANGE) {
            device->pad_ = static_cast<unsigned short>(SendMessageA(
                reinterpret_cast<HWND>(lparam), CB_GETCURSEL, 0, 0));
            SendMessageA(GetDlgItem(dialog, 0x3ec), CB_SETCURSEL,
                         device->pads_[device->pad_].selected_, 0);
            return TRUE;
          }
          break;
      }
      break;
  }
  return FALSE;
}

// 0x404030
int HLDevSet::Assign(const HLDevSet& other) {
  if (entries_ == NULL) {
    if (!Alloc()) {
      return 0;
    }
  }
  count_ = other.count_;
  selected_ = other.selected_;
  kind_ = other.kind_;
  guid_ = other.guid_;
  memcpy(entries_, other.entries_, kMaxEntries * sizeof(Entry));
  return 1;
}

// 0x404980
void HLDevice::LoadSettings() {
  app_->profile.SetSection("DirectX");
  if (app_->options & AppSettings::kScreenChoice) {
    if (app_->profile.GetBool("FullScreen",
                              app_->flags & AppSettings::kFullScreen)) {
      app_->flags |= AppSettings::kFullScreen;
    } else {
      app_->flags &= ~AppSettings::kFullScreen;
    }
  }
  flags_ |= kFlagRemember;
  if (app_->graphics_flags & AppSettings::kDirectDraw) {
    app_->profile.ReadString("DrawDevice", g_text_buffer, sizeof(g_text_buffer),
                             "");
    if (!graphics_.Select(g_text_buffer)) {
      flags_ &= ~kFlagRemember;
    }
  }
  if (app_->graphics_flags & AppSettings::kSound) {
    app_->profile.ReadString("SoundDevice", g_text_buffer,
                             sizeof(g_text_buffer), "");
    if (!sound_.Select(g_text_buffer)) {
      flags_ &= ~kFlagRemember;
    }
  }
  if (app_->graphics_flags & AppSettings::kInput) {
    char key[] = "InputDevice1";
    for (int i = 0; i < pad_count_; ++i) {
      app_->profile.GetString(key, "", g_text_buffer, sizeof(g_text_buffer));
      if (!pads_[i].Select(g_text_buffer)) {
        flags_ &= ~kFlagRemember;
        pads_[i].Select(static_cast<unsigned char>(i));
      }
      ++key[sizeof(key) - 2];
    }
  }
  if (app_->profile.GetBool("ViewDialog", 1)) {
    flags_ &= ~kFlagRemember;
  }
}

// 0x404bb0
void HLDevice::SaveSettings() {
  app_->profile.SetSection("DirectX");
  if (app_->options & AppSettings::kScreenChoice) {
    app_->profile.WriteBool("FullScreen",
                            app_->flags & AppSettings::kFullScreen);
  }
  app_->profile.WriteBool("ViewDialog", !(flags_ & kFlagRemember));
  if (app_->graphics_flags & AppSettings::kDirectDraw) {
    const char* name = graphics_.selected_name();
    app_->profile.WriteString("DrawDevice", name);
    app_->graphics->set_device(graphics_.guid());
    Log("DrawDevice=%s", name);
  }
  if (app_->graphics_flags & AppSettings::kSound) {
    const char* name = sound_.selected_name();
    app_->profile.WriteString("SoundDevice", name);
    Log("SoundDevice=%s", name);
  }
  if (app_->graphics_flags & AppSettings::kInput) {
    char key[] = "InputDevice1";
    for (int i = 0; i < pad_count_; ++i) {
      app_->input->set_pad_guid(i, pads_[i].guid());
      const char* name = pads_[i].selected_name();
      app_->profile.WriteString(key, name);
      Log("%s=%s", key, name);
      ++key[sizeof(key) - 2];
    }
  }
}
