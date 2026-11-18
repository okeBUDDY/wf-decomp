#include "file.h"

static const char* kModes[] = {"rb", "wb", "ab", "rb+"};  // 0x45d16c

// 0x401cc0
int File::IsOpen() { return file_ != NULL; }

// 0x401cd0
int File::Read(void* buffer, int size) {
  if (file_ != NULL) {
    return fread(buffer, 1, size, file_);
  }
  return 0;
}

// 0x401d00
int File::Write(const void* buffer, int size) {
  if (file_ != NULL) {
    return fwrite(buffer, 1, size, file_);
  }
  return 0;
}

// 0x401d30
void File::Flush() {
  if (file_ != NULL) {
    fflush(file_);
  }
}

// 0x401d40
int File::Seek(long offset, int origin) {
  if (file_ != NULL) {
    return fseek(file_, offset, origin);
  }
  return 0;
}

// 0x401d70
long File::Tell() {
  if (file_ != NULL) {
    return ftell(file_);
  }
  return 0;
}

// 0x401d90
int File::Open(const char* path, int mode) {
  file_ = fopen(path, kModes[mode]);
  return file_ != NULL;
}

// 0x401dc0
long File::Size() {
  if (file_ != NULL) {
    long pos = Tell();
    Seek(0, SEEK_END);
    long size = Tell();
    Seek(pos, SEEK_SET);
    return size;
  }
  return 0;
}
