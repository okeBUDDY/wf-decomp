// HLDevice and HLDevSet: the device choices of the start-up dialog.
#include "hl_device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
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
    if (!(app_->options & 0x8)) {
      EnableWindow(GetDlgItem(dialog, 0x3e9), FALSE);
    }
  } else {
    CheckRadioButton(dialog, 0x3e9, 0x3ea, 0x3e9);
    if (!(app_->options & 0x8)) {
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
