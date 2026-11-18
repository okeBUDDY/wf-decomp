#include "path_util.h"

#include <string.h>

// 0x4022e0
const char* GetFileName(const char* path) {
  for (const char* p = path + strlen(path); p > path; --p) {
    if (*p == '\\' || *p == '/') {
      return p + 1;
    }
  }
  return path;
}

// 0x402320
int GetFileWriteTime(const char* path, FILETIME* time) {
  HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    return 0;
  }
  int result = GetFileTime(file, NULL, NULL, time) != 0;
  CloseHandle(file);
  return result;
}
