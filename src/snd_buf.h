// CSndBuf: one DirectSound buffer filled from a CSndFile (vtable 0x45226c),
// kept in the sound system's CSndBufList.
#ifndef WF_SND_BUF_H_
#define WF_SND_BUF_H_

#include <windows.h>

#include <dsound.h>
#include <mmsystem.h>

#include "object.h"

class CSndBuf;
class CSndFile;
class CSndBufList;

// The list link of a CSndBuf. Its inline constructor runs before CSndBuf
// stores its vtable, which fixes the order of the stores in CSndBuf().
class CSndBufLink : public Object {
 protected:
  friend class CSndBufList;
  friend class SoundSystem;
  friend class Channel;

  CSndBufLink() {
    id_ = -1;
    next_ = NULL;
    prev_ = NULL;
  }

  int id_;         // 0x08, -1 while not in the list
  CSndBuf* prev_;  // 0x0c
  CSndBuf* next_;  // 0x10
};

class CSndBuf : public CSndBufLink {
 public:
  enum Flags {
    kChannelMask = 0x3,
    kLoaded = 0x100,
    kBufferInUse = 0x200,  // buffer_ handed out by GetBuffer()
    kLoop = 0x400,         // CSndStrm: the file restarts at its end
    kPrefill = 0x800,      // CSndStrm: the buffer is still being filled
    kFromDisk = 0x1000000,
    kUserFlags = 0xff000100,
  };

  CSndBuf();  // 0x40e1d0

  // 0x40e210
  virtual const char* GetName() { return "CSndBuf"; }
  virtual void Release();  // 0x40e230
  // Called when the sound system pauses.
  virtual void Suspend() {}
  // Reloads the samples after the buffer was lost.
  virtual void Restore();  // 0x40e720
  // Loads `path` into a new buffer; `channel` (0..3) is kept in the flags.
  virtual int Load(const char* path, unsigned int flags,
                   unsigned int channel);  // 0x40e610
  // 0x40e220
  virtual const char* GetPath() { return path_ != NULL ? path_ : ""; }
  // Returns the buffer, or a duplicate when it is already in use.
  virtual IDirectSoundBuffer* GetBuffer();  // 0x40de80
  // Gives back a buffer returned by GetBuffer().
  virtual void ReleaseBuffer(IDirectSoundBuffer* buffer);  // 0x40ded0

  // Volume group (0..3) of the buffer.
  int group() const { return flags_ & 0xff; }
  unsigned int loaded() const { return flags_ & kLoaded; }

 protected:
  friend class CSndBufList;
  friend class SoundSystem;
  friend class Channel;

  int CreateBuffer(WAVEFORMATEX* format, int size);  // 0x40def0
  CSndFile* OpenSndFile(const char* path,
                        WAVEFORMATEX* format);  // 0x40e2a0
  // Copies `size` bytes of `file` to `out`; with `loop` the file restarts
  // at its end, otherwise the rest is cleared.
  int Fill(void* out, CSndFile* file, int offset, int size,
           int loop);  // 0x40e420
  // Writes `size` bytes (-1: all) of `file` at `offset` of the buffer.
  int Write(CSndFile* file, int offset, int size, int loop);  // 0x40e4b0
  int SetPath(const char* path, CSndFile* file,
              WAVEFORMATEX* format);  // 0x40e560

  IDirectSoundBuffer* buffer_;  // 0x14
  unsigned int flags_;          // 0x18
  int size_;                    // 0x1c, bytes
  float seconds_;               // 0x20
  char* path_;                  // 0x24
};

class SoundSystem;
extern SoundSystem* g_sound_system;  // 0x468f98
extern CSndBufList* g_snd_buf_list;  // 0x468f9c

#include "snd_buf_list.h"

#endif  // WF_SND_BUF_H_
