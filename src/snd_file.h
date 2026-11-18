// CSndFile: a sound source decoded into PCM (vtable 0x45220c), with one
// subclass per format. CSndBuf::OpenSndFile picks the class from the last
// letter of the file name: CSndFileWAV ("*.wav"), CSndFileOGG ("*.ogg") and
// CSndFileSEQ ("*.seq").
#ifndef WF_SND_FILE_H_
#define WF_SND_FILE_H_

#include <windows.h>

#include <mmsystem.h>
#include <vorbis/vorbisfile.h>

#include "file.h"

class CSndFile {
 public:
  CSndFile() : file_(NULL), size_(0) {}
  // 0x40e020, scalar deleting dtor 0x40e030
  virtual ~CSndFile() {}
  virtual const char* GetName() = 0;
  // Opens `path` (from data.bin unless `from_disk`) and fills `format`.
  virtual int Open(const char* path, WAVEFORMATEX* format, int from_disk) = 0;
  virtual void Close() = 0;
  // Decodes up to `size` bytes; returns the number of bytes read.
  virtual int Read(void* buffer, int size) = 0;
  // Moves to byte `position` of the PCM data.
  virtual void Seek(int position) = 0;

  int size() const { return size_; }
  // Seeks to `position` modulo the PCM size.
  void SeekLooped(int position) {
    if (size_ != 0) {
      while (position >= size_) {
        position -= size_;
      }
      Seek(position);
    }
  }

 protected:
  File* file_;  // 0x04
  int size_;    // 0x08, PCM bytes
};

class CSndFileWAV : public CSndFile {
 public:
  // 0x40e0e0, scalar deleting dtor 0x40e0c0
  virtual ~CSndFileWAV() { Close(); }
  // 0x40e080
  virtual const char* GetName() { return "CSndFileWAV"; }
  virtual int Open(const char* path, WAVEFORMATEX* format,
                   int from_disk);  // 0x40f390
  // 0x40e050
  virtual void Close() {
    if (file_ != NULL) {
      file_->Close();
      delete file_;
      file_ = NULL;
    }
  }
  // 0x40e090
  virtual int Read(void* buffer, int size) { return file_->Read(buffer, size); }
  // 0x40e0a0
  virtual void Seek(int position) {
    file_->Seek(data_offset_ + position, SEEK_SET);
  }

 private:
  long data_offset_;  // 0x0c, file offset of the "data" chunk
};

class CSndFileOGG : public CSndFile {
 public:
  // 0x40e180, scalar deleting dtor 0x40e160
  virtual ~CSndFileOGG() { Close(); }
  // 0x40e150
  virtual const char* GetName() { return "CSndFileOGG"; }
  virtual int Open(const char* path, WAVEFORMATEX* format,
                   int from_disk);           // 0x40e8f0
  virtual void Close();                      // 0x40e8b0
  virtual int Read(void* buffer, int size);  // 0x40e7d0
  virtual void Seek(int position);           // 0x40e790

 private:
  int reserved_;           // 0x0c
  OggVorbis_File vorbis_;  // 0x10
  int block_align_;        // 0x2e0
  int position_;           // 0x2e4
};

#endif  // WF_SND_FILE_H_
