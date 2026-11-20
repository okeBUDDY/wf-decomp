// CSndStrm: a CSndBuf that streams its file through a one second buffer
// (vtable 0x452358).
#ifndef WF_SND_STRM_H_
#define WF_SND_STRM_H_

#include "snd_buf.h"

class CSndStrm : public CSndBuf {
 public:
  CSndStrm();  // 0x40f9b0

  // 0x40f9d0
  virtual const char* GetName() { return "CSndStrm"; }
  virtual void Release();  // 0x40fb00
  virtual void Restore();  // 0x40fb90
  virtual int Load(const char* path, unsigned int flags,
                   unsigned int group);  // 0x40f9e0
  // The path of the playing SEQ piece, or of the file.
  virtual const char* GetPath();  // 0x40fc10
  // The buffer can be played by one channel at a time.
  virtual IDirectSoundBuffer* GetBuffer();  // 0x40f8a0

  // Refills the part of the buffer that was played; returns 0 on failure.
  int Update();  // 0x40f8c0
  // Whether the buffer holds enough to start playing.
  int ready() const { return (flags_ & kPrefill) == 0; }
  // Rewinds the file and starts filling the buffer.
  void SetLoop(int loop);  // 0x40fc50

 private:
  CSndFile* file_;           // 0x28
  unsigned int position_;    // 0x2c, write position in the buffer
  unsigned int block_size_;  // 0x30, bytes written while prefilling
};

#endif  // WF_SND_STRM_H_
