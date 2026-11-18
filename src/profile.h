// Profile: the game's .ini settings file (Application::profile).
#ifndef WF_PROFILE_H_
#define WF_PROFILE_H_

#include <windows.h>

#include <string.h>

class Profile {
 public:
  enum { kSectionSize = 64, kValueSize = 32 };

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
  // Uses "<name>.ini" next to the executable. Returns 0 without memory.
  int Open(const char* name);  // 0x406910
  // Reads `key` as a number (any base strtol takes), or returns `def`.
  int GetInt(const char* key, int def);  // 0x40cd00
  // Writes `value` printed with `format` to `key`.
  void WriteInt(const char* key, int value, const char* format);  // 0x40cd50

 private:
  int unknown_00_;
  char* path_;     // 0x04
  char* section_;  // 0x08
  // 0x0c, kValueSize bytes. It starts kValueSize bytes into the section
  // buffer, so long section names run into it.
  char* value_;
};

#endif  // WF_PROFILE_H_
