// SoundSystem ("HLSound", vtable 0x451dec): DirectSound, the primary buffer,
// the CSndBuf list and a fixed array of playback channels updated by a
// SoundThread.
#ifndef WF_SND_SYS_H_
#define WF_SND_SYS_H_

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "application.h"
#include "object.h"
#include "snd_buf_list.h"
#include "thread.h"

class CSndBuf;
class SoundSystem;

// One playing sound (vtable 0x451de4).
class Channel {
 public:
  enum Flags {
    kLoop = 0x10000,
    kFading = 0x4000000,
    kStopAfterFade = 0x8000000,
  };

  // 0x408ef0
  Channel() {
    sound_ = NULL;
    buffer_ = NULL;
    next_ = NULL;
    prev_ = NULL;
    flags_ = 0;
  }
  // 0x4094c0, vector deleting dtor 0x409780
  virtual ~Channel() { Stop(); }

  // 0x4093e0
  void Stop() {
    if (system_ != NULL) {
      Lock();
      if (sound_ != NULL) {
        Release();
      }
      Unlock();
    }
  }
  // Sets the volume (0..1, scaled by the group volume); a negative value
  // reapplies the current one.
  void SetVolume(float volume);  // 0x408f10
  void Release();                // 0x409370
  int Play();                    // 0x409500
  // Pan from -1 (left) to 1 (right).
  void SetPan(float pan);  // 0x40f4c0
  void SetVolumeLocked(float volume);  // 0x40f530
  // Fades from `from` to `to` (negative: current volume) in `time` seconds,
  // then stops the channel when `stop` is set.
  void Fade(float from, float to, float time, int stop);  // 0x40f570
  int Start();  // 0x40f650

  static SoundSystem* system_;  // 0x468fa8

 private:
  friend class SoundSystem;

  static void Lock();
  static void Unlock();

  IDirectSoundBuffer* buffer_;  // 0x04
  CSndBuf* sound_;              // 0x08
  unsigned int flags_;          // 0x0c
  Channel* prev_;               // 0x10
  Channel* next_;               // 0x14
  float volume_;                // 0x18
  float pan_;                   // 0x1c
  int reserved_20_;             // 0x20
  float fade_time_;             // 0x24
  float fade_elapsed_;          // 0x28
  float fade_from_;             // 0x2c
  float fade_delta_;            // 0x30
};

// Mixes the channels every few milliseconds (vtable 0x451ddc).
class SoundThread : public Thread {
 public:
  SoundThread() : system_(NULL) {}
  virtual int Run();  // 0x409270

  SoundSystem* system_;  // 0x14
  float time_;           // 0x18, seconds
};

class SoundSystem : public Object {
 public:
  SoundSystem();  // 0x408f90
  // 0x409010, scalar deleting dtor 0x408ff0
  virtual ~SoundSystem() {}
  virtual int Initialize(Application* app);  // 0x409580
  virtual void Finalize();                   // 0x409810
  virtual void Method3();                    // 0x4098e0
  virtual void Method4();                    // 0x409980

  // Stops every channel playing `buffer`; without a buffer, every channel
  // of `group` (-1: all channels).
  void Stop(CSndBuf* buffer, int group);  // 0x409420
  // Sets the volume of `group` (-1: every group).
  void SetGroupVolume(float volume, int group);  // 0x409030

 private:
  int SetFormat(int rate, int bits, int channels);  // 0x4090e0
  int CreateMixer();                                // 0x408cb0

 public:
  GUID* device_;                 // 0x08
  int reserved_0c_;              // 0x0c
  int channel_count_;            // 0x10

  unsigned int muted_groups_;  // 0x14, bit per CSndBuf group
  float group_volume_[4];      // 0x18
  IDirectSound* dsound_;       // 0x28, NULL without DirectSound
  IDirectSoundBuffer* primary_;  // 0x2c
  Application* app_;             // 0x30
  Channel* channels_;            // 0x34
  Channel* head_;                // 0x38, playing channels
  Channel* tail_;                // 0x3c
  int reserved_40_;              // 0x40
  SoundThread thread_;           // 0x44
  CSndBufList buffers_;          // 0x60
  WAVEFORMATEX format_;          // 0x70
  CRITICAL_SECTION lock_;        // 0x84
};

inline void Channel::Lock() { EnterCriticalSection(&system_->lock_); }
inline void Channel::Unlock() { LeaveCriticalSection(&system_->lock_); }

#endif  // WF_SND_SYS_H_
