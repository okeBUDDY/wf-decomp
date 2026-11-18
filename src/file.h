// File: a stdio file behind a virtual interface (vtable 0x4514cc), and
// PackFile: a file stored inside the data.bin archive (vtable 0x451eec).
#ifndef WF_FILE_H_
#define WF_FILE_H_

#include <stdio.h>

class File {
 public:
  enum Mode { kRead, kWrite, kAppend, kReadUpdate };

  // scalar deleting dtor 0x401f40
  virtual ~File() {
    if (file_ != NULL) {
      fclose(file_);
    }
    file_ = NULL;
  }
  virtual int Open(const char* path, int mode);     // 0x401d90
  virtual void Close();                             // 0x401ac0
  virtual int IsOpen();                             // 0x401cc0
  virtual int Read(void* buffer, int size);         // 0x401cd0
  virtual int Write(const void* buffer, int size);  // 0x401d00
  virtual void Flush();                             // 0x401d30
  virtual int Seek(long offset, int origin);        // 0x401d40
  virtual long Tell();                              // 0x401d70
  virtual long Size();                              // 0x401dc0

 protected:
  FILE* file_;  // 0x04
};

class PackFile : public File {
 public:
  virtual ~PackFile();  // 0x40a1b0, scalar deleting dtor 0x40a290
  // Opens the archive member `name`; only File::kRead is supported.
  virtual int Open(const char* name, int mode);     // 0x40dbb0 (pack.cpp)
  virtual void Close();                             // 0x409b40
  virtual int Read(void* buffer, int size);         // 0x409b70
  virtual int Write(const void* buffer, int size);  // 0x40a1f0
  virtual void Flush();                             // shares 0x406900
  virtual int Seek(long offset, int origin);        // 0x40a220
  virtual long Tell();                              // 0x40a200
  virtual long Size();                              // 0x40a210

 private:
  long offset_;  // 0x08, start of the member in data.bin
  long pos_;     // 0x0c
  long size_;    // 0x10
};

#endif  // WF_FILE_H_
