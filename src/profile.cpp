#include "profile.h"

#include <stdio.h>
#include <stdlib.h>

// 0x40cd00
int Profile::GetInt(const char* key, int def) {
  if (path_ != NULL && section_[0] != '\0') {
    GetPrivateProfileString(section_, key, "", value_, kValueSize, path_);
    if (value_[0] != '\0') {
      return strtol(value_, NULL, 0);
    }
  }
  return def;
}

// 0x40cd50
void Profile::WriteInt(const char* key, int value, const char* format) {
  if (path_ != NULL && section_[0] != '\0') {
    sprintf(value_, format, value);
    WritePrivateProfileString(section_, key, value_, path_);
  }
}
