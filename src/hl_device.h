// HLDevice: the system at Application+0xa0 (vtable 0x451848) that lets the
// player pick the graphics, sound and input devices and the screen mode in
// the start-up dialog, and keeps the choices.
#ifndef WF_HL_DEVICE_H_
#define WF_HL_DEVICE_H_

#include <windows.h>

#include "system.h"

struct Application;

// The devices of one kind to choose from (vtable 0x451834).
class HLDevSet : public Object {
 public:
  enum { kMaxEntries = 8 };
  // Entry::kind.
  enum Kind {
    kDefault = 0,  // the default device
    kGuid = 1,     // the device `guid`
    kNamed = 2,    // known by its name only
  };

  struct Entry {
    int kind;       // 0x00, Kind
    GUID guid;      // 0x04
    char name[64];  // 0x14
  };

  HLDevSet() {  // 0x404700
    name_ = "HLDevSet";
    count_ = selected_ = 0;
    kind_ = kDefault;
    entries_ = NULL;
  }

  // Adds a device entry: `guid` NULL is the default device; `named` uses
  // `name` as is.
  void Add(const GUID* guid, const char* name, int named);  // 0x404730
  // Selects the entry called `name`; 0 when there is none.
  int Select(const char* name);  // 0x4040c0
  // Selects entry `index` (the last one when it is past the end).
  void Select(unsigned char index);  // 0x404180
  // Lists the entries in the combo box `combo` and selects the current
  // one; without `enabled` the box only says the device is not used.
  void Fill(HWND combo, int enabled);  // 0x4041e0
  // Selects the entry chosen in the combo box `combo`.
  void ReadSelection(HWND combo);  // 0x404280
  // Frees the entries.
  void Clear() {
    if (entries_ != NULL) {
      free(entries_);
    }
    entries_ = NULL;
  }

  unsigned short count() const { return count_; }

 private:
  // Copies `name` to `out` without its leading and trailing spaces, at most
  // 63 characters.
  void CopyName(char* out, const char* name);  // 0x4042f0

  unsigned short count_;     // 0x08
  unsigned short selected_;  // 0x0a
  int kind_;                 // 0x0c, of the selected entry
  GUID guid_;                // 0x10, of the selected entry
  Entry* entries_;           // 0x20
};

class HLDevice : public System {
 public:
  enum Flags {
    kFlagRemember = 0x1,  // the dialog's check box 0x3ef
  };

  HLDevice();                                // 0x404840
  virtual ~HLDevice();                       // 0x404910
  virtual int Initialize(Application* app);  // 0x404f90
  virtual void Finalize();                   // 0x404350
  virtual void Pause() {}
  virtual void Resume() {}

 private:
  // Returns 0 (with a warning on `window`) when the desktop has 256 colors
  // and the game runs in a window; else takes its depth.
  int CheckDisplay(HWND window);  // 0x4043f0
  // Fills the controls of the dialog from the choices.
  void InitDialog(HWND dialog);  // 0x404480
  // Reads the choices back from the dialog.
  void ReadDialog(HWND dialog);  // 0x4045e0

  Application* app_;          // 0x08
  unsigned int flags_;        // 0x0c, Flags
  HLDevSet graphics_;         // 0x10
  HLDevSet sound_;            // 0x34
  HLDevSet* pads_;            // 0x58, new[]ed, pad_count_
  unsigned short pad_count_;  // 0x5c
  unsigned short pad_;        // 0x5e, the pad shown in the dialog
};

#endif  // WF_HL_DEVICE_H_
