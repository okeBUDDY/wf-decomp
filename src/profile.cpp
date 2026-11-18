#include "profile.h"

#include <stdio.h>
#include <stdlib.h>

#include "path_util.h"

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

// 0x4046b0
char* Profile::ReadString(const char* key, char* out, int size,
                          const char* def) {
  if (path_ != NULL && section_[0] != '\0') {
    GetPrivateProfileString(section_, key, def, out, size, path_);
  } else {
    strcpy(out, def);
  }
  return out;
}

// 0x40cd50
void Profile::WriteInt(const char* key, int value, const char* format) {
  if (path_ != NULL && section_[0] != '\0') {
    sprintf(value_, format, value);
    WritePrivateProfileString(section_, key, value_, path_);
  }
}

// 0x406910
int Profile::Open(const char* name) {
  path_ = new char[MAX_PATH + kValueSize + kSectionSize];
  if (path_ == NULL) {
    return 0;
  }
  section_ = path_ + MAX_PATH;
  value_ = section_ + kValueSize;
  GetModuleFileName(NULL, path_, MAX_PATH);
  sprintf(const_cast<char*>(GetFileName(path_)), "%s.ini", name);
  section_[0] = '\0';
  return 1;
}
