#include "file.h"

// 0x409b40
void PackFile::Close() {
  if (file_ != NULL) {
    fclose(file_);
  }
  file_ = NULL;
  pos_ = 0;
  size_ = 0;
}

// 0x409b70
int PackFile::Read(void* buffer, int size) {
  if (file_ != NULL) {
    if (size > size_ - pos_) {
      size = size_ - pos_;
    }
    pos_ += size;
    return fread(buffer, 1, size, file_);
  }
  return 0;
}

// 0x40a1b0
PackFile::~PackFile() { Close(); }

// 0x40a1f0
int PackFile::Write(const void* buffer, int size) { return 0; }

// 0x406900 (an empty function shared by many classes)
void PackFile::Flush() {}

// 0x40a200
long PackFile::Tell() { return pos_; }

// 0x40a210
long PackFile::Size() { return size_; }

// 0x40a220
int PackFile::Seek(long offset, int origin) {
  if (file_ != NULL) {
    switch (origin) {
      case SEEK_SET:
        pos_ = offset;
        break;
      case SEEK_CUR:
        pos_ = pos_ + offset;
        break;
      case SEEK_END:
        pos_ = size_ + offset;
        break;
      default:
        return -1;
    }
    if (pos_ > size_) {
      pos_ = size_;
    } else if (pos_ < 0) {
      pos_ = 0;
    }
    return fseek(file_, offset_ + pos_, SEEK_SET);
  }
  return -1;
}
