// HLInput: polls the players' pads, a DirectInput 7 keyboard or joystick
// each (vtable 0x451a50).
#ifndef WF_HL_INPUT_H_
#define WF_HL_INPUT_H_

#define DIRECTINPUT_VERSION 0x0700
#include <windows.h>

#include <dinput.h>

#include "system.h"
#include "task_list.h"

struct Application;
class HLDevSet;
class HLInput;
class Profile;

// One input device: eight directions and up to eight trigger buttons.
class HLInpDev : public Object {
 public:
  enum Buttons {
    kUp = 0x1,
    kDown = 0x2,
    kLeft = 0x4,
    kRight = 0x8,
    kTrigger = 0x100,  // trigger i is kTrigger << i
  };
  enum { kMaxTriggers = 8 };

  HLInpDev() {
    name_ = "HLInpDev";
    buttons_ = pressed_ = previous_ = 0;
    button_count_ = 0;
    device_ = NULL;
  }

  // Returns the first trigger code held down, or -1.
  virtual int GetPressed() = 0;
  // Writes the name of trigger code `code` to `out`.
  virtual void GetName(int code, char* out) = 0;
  // Creates the device `guid` (unused by the keyboard). Returns 0 on error.
  virtual int Open(HLInput* input, const GUID* guid) = 0;
  // Reads the device state into buttons_.
  virtual void Poll() = 0;

  unsigned short buttons() const { return buttons_; }
  unsigned short pressed() const { return pressed_; }

 protected:
  friend class HLInput;
  friend class InputTask;

  // Reads trigger_map_ from the profile's [TriggerMap] entry for the
  // device and clamps button_count_ to the device's buttons.
  void LoadTriggerMap();  // 0x405fe0
  // Writes trigger_map_ back to the profile.
  void SaveTriggerMap();  // 0x406170

  static Profile* profile_;  // 0x468b8c

  IDirectInputDevice7* device_;              // 0x08
  unsigned short buttons_;                   // 0x0c, Buttons held down
  unsigned short pressed_;                   // 0x0e, Buttons pressed this frame
  unsigned short previous_;                  // 0x10, buttons_ of the last frame
  unsigned short button_count_;              // 0x12, triggers used
  unsigned char trigger_map_[kMaxTriggers];  // 0x14, device code of each
};

// The system keyboard (vtable 0x451be8).
class HLKeyboard : public HLInpDev {
 public:
  virtual int GetPressed();                            // 0x405d00
  virtual void GetName(int code, char* out);           // 0x405d80
  virtual int Open(HLInput* input, const GUID* guid);  // 0x406420
  virtual void Poll();                                 // 0x405c00

 private:
  static unsigned char state_[256];        // 0x468b90, read by Poll()
  static unsigned char scan_[256];         // 0x468c90, read by GetPressed()
  static int trigger_keys_[kMaxTriggers];  // 0x45d1d8, keys GetPressed() knows
};

// A joystick (vtable 0x451bd4).
class HLJoystick : public HLInpDev {
 public:
  virtual int GetPressed();                            // 0x405b70
  virtual void GetName(int code, char* out);           // 0x405fc0
  virtual int Open(HLInput* input, const GUID* guid);  // 0x406260
  virtual void Poll();                                 // 0x405ab0
};

// Polls the pads every frame, even while the time is stopped (vtable
// 0x451a3c).
class InputTask : public Task {
 public:
  InputTask() {
    enabled_ = 0;
    input_ = NULL;
  }
  virtual int Initialize();  // 0x405e30
  virtual void Finalize() {}
  // Polls the pads; never lets the task be removed.
  virtual int IsProtected();  // 0x405f30
  virtual void Run() {}

  HLInput* input_;  // 0x14
};

class HLInput : public System {
 public:
  enum { kMaxPads = 4 };

  HLInput();                                 // 0x405e40
  virtual int Initialize(Application* app);  // 0x406530
  virtual void Finalize();                   // 0x406750
  virtual void Pause() {}
  virtual void Resume() {}

  // Adds the attached joysticks and the keyboard to `set`. Returns 0 when
  // there is none or DirectInput is missing.
  int EnumDevices(Application* app, HLDevSet* set);  // 0x405ea0

  HLInpDev* pad(int index) const { return pads_[index]; }
  unsigned short pad_count() const { return pad_count_; }
  // Uses the device `guid` (NULL: the keyboard) for pad `index`.
  void set_pad_guid(int index, const GUID* guid) { guids_[index] = guid; }
  // The buttons that stop and restart the time (0: none).
  void set_time_stop_mask(unsigned short mask) { time_stop_mask_ = mask; }

 private:
  friend class HLJoystick;
  friend class HLKeyboard;
  friend class InputTask;

  static BOOL CALLBACK EnumDeviceProc(LPCDIDEVICEINSTANCE instance,
                                      LPVOID context);  // 0x405a60

  IDirectInput7* dinput_;          // 0x08
  unsigned short pad_count_;       // 0x0c
  unsigned short max_buttons_;     // 0x0e, triggers per pad
  HLInpDev* pads_[kMaxPads];       // 0x10, may share a device
  const GUID* guids_[kMaxPads];    // 0x20, NULL: the keyboard
  Application* app_;               // 0x30
  HLInpDev* devs_[kMaxPads];       // 0x34, owned
  InputTask* task_;                // 0x44
  unsigned short time_stop_mask_;  // 0x48, buttons that stop the time
};

#endif  // WF_HL_INPUT_H_
