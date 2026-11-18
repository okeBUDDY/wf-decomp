// Profile: the game's .ini settings file (Application::profile).
#ifndef WF_PROFILE_H_
#define WF_PROFILE_H_

#include <windows.h>

#include <string.h>

// A scratch buffer for profile values and paths.
extern char g_text_buffer[256];  // 0x468d90

class Profile {
 public:
  enum { kSectionSize = 64, kValueSize = 32 };

  Profile() { path_ = section_ = value_ = NULL; }
  virtual ~Profile() {
    if (path_ != NULL) {
      delete[] path_;
    }
    path_ = section_ = value_ = NULL;
  }

  // Selects the [section] later reads come from.
  void SetSection(const char* section) {
    if (section_ != NULL) {
      strncpy(section_, section, kSectionSize - 1);
      section_[kSectionSize - 1] = '\0';
    }
  }
  // Reads `key` of the current section, or copies `def` without a file.
  void GetString(const char* key, const char* def, char* out, int size) {
    if (path_ != NULL && section_[0] != '\0') {
      GetPrivateProfileString(section_, key, def, out, size, path_);
    } else {
      strcpy(out, def);
    }
  }
  // Reads `key` of the current section into `out`, or copies `def` without
  // a file. Returns `out`.
  char* ReadString(const char* key, char* out, int size,
                   const char* def);  // 0x4046b0
  // Writes `value` to `key` of the current section.
  void WriteString(const char* key, const char* value) {
    if (path_ != NULL && section_[0] != '\0') {
      WritePrivateProfileString(section_, key, value, path_);
    }
  }
  // Writes `value` as "1" or "0" to `key` of the current section.
  void WriteBool(const char* key, int value) {
    if (path_ != NULL && section_[0] != '\0') {
      WritePrivateProfileString(section_, key, value ? "1" : "0", path_);
    }
  }
  // Uses "<name>.ini" next to the executable. Returns 0 without memory.
  int Open(const char* name);  // 0x406910
  // Reads `key` as a number (any base strtol takes), or returns `def`.
  int GetInt(const char* key, int def);  // 0x40cd00
  // Reads `key` as a flag (anything but "0" is set), or returns `def`
  // as 0 or 1. Emitted out of line by hl_device.cpp (0x404660).
  int GetBool(const char* key, int def) {
    if (path_ != NULL && section_[0] != '\0') {
      GetPrivateProfileString(section_, key, "", value_, kValueSize, path_);
      if (value_[0] != '\0') {
        return value_[0] != '0';
      }
    }
    return def != 0;
  }
  // Writes `value` printed with `format` to `key`.
  void WriteInt(const char* key, int value, const char* format);  // 0x40cd50

 private:
  char* path_;     // 0x04
  char* section_;  // 0x08
  // 0x0c, kValueSize bytes. It starts kValueSize bytes into the section
  // buffer, so long section names run into it.
  char* value_;
};

#endif  // WF_PROFILE_H_
