// Profile: the game's .ini settings file (GetPrivateProfileString wrapper).
// Only the static state and the inline readers used by decompiled code are
// known so far.
#ifndef WF_PROFILE_H_
#define WF_PROFILE_H_

#include <windows.h>

#include <string.h>

class Profile {
 public:
  enum { kSectionSize = 64 };

  // Selects the [section] later reads come from.
  static void SetSection(const char* section) {
    if (section_ != NULL) {
      strncpy(section_, section, kSectionSize - 1);
      section_[kSectionSize - 1] = '\0';
    }
  }
  // Reads `key` of the current section, or copies `def` without a file.
  static void GetString(const char* key, const char* def, char* out, int size) {
    if (path_ != NULL && section_[0] != '\0') {
      GetPrivateProfileString(section_, key, def, out, size, path_);
    } else {
      strcpy(out, def);
    }
  }

 private:
  static char* path_;     // 0x4673ec
  static char* section_;  // 0x4673f0
};

#endif  // WF_PROFILE_H_
