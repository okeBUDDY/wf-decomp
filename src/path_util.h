// File name helpers.
#ifndef WF_PATH_UTIL_H_
#define WF_PATH_UTIL_H_

#include <windows.h>

// Returns the part of `path` after the last '\' or '/'.
const char* GetFileName(const char* path);  // 0x4022e0
// Reads the last write time of `path`; returns 0 on failure.
int GetFileWriteTime(const char* path, FILETIME* time);  // 0x402320

#endif  // WF_PATH_UTIL_H_
