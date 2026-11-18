#include "snd_sys.h"

#include <string.h>

#include "hl_device.h"
#include "snd_buf.h"

static char kName[] = "HLSound";  // 0x45d25c

SoundSystem* g_sound_system;  // 0x468f98
CSndBufList* g_snd_buf_list;  // 0x468f9c

void ResetStreams();  // 0x40ea20

// 0x408e40
BOOL CALLBACK SoundSystem::EnumDeviceProc(GUID* guid, LPCSTR description,
                                          LPCSTR module, void* context) {
  IDirectSound* dsound;
  if (DirectSoundCreate(guid, &dsound, NULL) == DS_OK) {
    static_cast<HLDevSet*>(context)->Add(guid, description, 0);
    dsound->Release();
  }
  return TRUE;
}

// 0x408f90
SoundSystem::SoundSystem() {
  device_ = NULL;
  app_ = NULL;
  dsound_ = NULL;
  primary_ = NULL;
  channels_ = NULL;
  tail_ = NULL;
  head_ = NULL;
  state_ = 0;
  muted_groups_ = 0;
  reserved_40_ = 0;
  name_ = kName;
}

// 0x409030
void SoundSystem::SetGroupVolume(float volume, int group) {
  EnterCriticalSection(&lock_);
  Channel* channel = head_;
  if (group >= 0) {
    group_volume_[group] = volume;
    while (channel != NULL) {
      if (channel->sound_->group() == group) {
        channel->SetVolume(-1.0f);
      }
      channel = channel->next_;
    }
  } else {
    group_volume_[0] = volume;
    group_volume_[1] = volume;
    group_volume_[2] = volume;
    group_volume_[3] = volume;
    while (channel != NULL) {
      channel->SetVolume(-1.0f);
      channel = channel->next_;
    }
  }
  LeaveCriticalSection(&lock_);
}

// 0x4090e0
int SoundSystem::SetFormat(int rate, int bits, int channels) {
  format_.wFormatTag = WAVE_FORMAT_PCM;
  format_.nChannels = channels;
  format_.nSamplesPerSec = rate;
  format_.wBitsPerSample = bits;
  format_.nBlockAlign = (bits >> 3) * channels;
  format_.nAvgBytesPerSec = format_.nBlockAlign * rate;
  format_.cbSize = sizeof(WAVEFORMATEX);
  if (dsound_ != NULL) {
    if (primary_ != NULL) {
      primary_->Stop();
      primary_->Release();
      primary_ = NULL;
    }
    DSBUFFERDESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    IDirectSoundBuffer* primary;
    if (dsound_->CreateSoundBuffer(&desc, &primary, NULL) != DS_OK) {
      // "could not get the primary buffer"
      Error(
          "\314\337\327\262\317\330\312\336\257\314\247\202\314\216\346\223\276"
          "\216\270\224s");
      return 0;
    }
    if (primary->SetFormat(&format_) != DS_OK) {
      // "could not set the sound format"
      Error(
          "\273\263\335\304\336\314\253\260\317\257\304\202\314\220\335\222\350"
          "\216\270\224s");
      return 0;
    }
    primary->Release();
    if (!CreateMixer()) {
      return 0;
    }
  }
  // "output format"
  Log("\217o\227\315\314\253\260\317\257\304:%dch %dHz %dbit", channels, rate,
      bits);
  return 1;
}

// 0x409240
int SoundSystem::EnumDevices(HLDevSet* set) {
  if (DirectSoundEnumerate(EnumDeviceProc, set) != DS_OK) {
    return 0;
  }
  return set->count() != 0;
}

// 0x409330
int CSndBufList::Clear() {
  if (head_ != NULL) {
    for (CSndBuf* buffer = tail_; buffer != NULL; buffer = buffer->prev_) {
      buffer->Release();
    }
    tail_ = NULL;
    head_ = NULL;
  }
  count_ = 0;
  return 1;
}

// 0x409420
void SoundSystem::Stop(CSndBuf* buffer, int group) {
  EnterCriticalSection(&lock_);
  Channel* channel = head_;
  if (buffer != NULL) {
    while (channel != NULL) {
      Channel* next = channel->next_;
      if (channel->sound_ == buffer) {
        channel->Release();
      }
      channel = next;
    }
  } else if (group >= 0) {
    while (channel != NULL) {
      Channel* next = channel->next_;
      if (channel->sound_->group() == group) {
        channel->Release();
      }
      channel = next;
    }
  } else {
    while (channel != NULL) {
      Channel* next = channel->next_;
      channel->Release();
      channel = next;
    }
  }
  LeaveCriticalSection(&lock_);
}

// 0x409580
int SoundSystem::Initialize(Application* app) {
  app_ = app;
  channel_count_ = app->sound_channels;
  if (DirectSoundCreate(device_, &dsound_, NULL) == DS_OK) {
    if (dsound_->SetCooperativeLevel(app_->window, DSSCL_PRIORITY) != DS_OK) {
      // "could not set the DirectSound cooperative level"
      Error(
          "DirectSound\202\314\213\246\222\262\203\214\203x\203\213\202"
          "\252\220\335\222\350\202\305\202\253\202\334\202\271\202\361");
      return 0;
    }
  } else {
    // "DirectSound is not available"
    Log("DirectSound\202\315\227\230\227p\202\305\202\253\202\334\202\271\202"
        "\361");
    dsound_ = NULL;
  }
  if (!SetFormat(44100, 16, 2)) {
    return 0;
  }
  thread_.system_ = this;
  Channel::system_ = this;
  g_sound_system = this;
  g_snd_buf_list = &buffers_;
  channels_ = new Channel[channel_count_];
  if (channels_ == NULL) {
    // "out of memory"
    Error(
        "\203\201\203\202\203\212\202\252\221\253\202\350\202\334\202\271\202"
        "\361");
    return 0;
  }
  if (!buffers_.Clear()) {
    // "could not initialize the sound buffer list"
    Error(
        "\273\263\335\304\336\312\336\257\314\247\212\307\227\235\274\275\303"
        "\321\202\314\217\211\212\372\211\273\216\270\224s");
    return 0;
  }
  InitializeCriticalSection(&lock_);
  // Start the mixing thread.
  Thread* thread = &thread_;
  if (thread->handle_ != NULL) {
    thread->Stop();
  }
  thread->exit_code_ = 0;
  thread->running_ = 1;
  thread->handle_ =
      CreateThread(NULL, 0, Thread::ThreadProc, thread, 0, &thread->id_);
  if (thread->handle_ == NULL) {
    // "could not create the sound thread"
    Error(
        "\273\263\335\304\336\275\332\257\304\336\220\266\220\254\216\270\224"
        "s");
    return 0;
  }
  group_volume_[0] = 1.0f;
  group_volume_[1] = 1.0f;
  group_volume_[2] = 1.0f;
  group_volume_[3] = 1.0f;
  ResetStreams();
  return 1;
}

// 0x409810
void SoundSystem::Finalize() {
  Stop(NULL, -1);
  thread_.Stop();
  buffers_.ReleaseAll();
  if (channels_ != NULL) {
    delete[] channels_;
    channels_ = NULL;
  }
  thread_.system_ = NULL;
  Channel::system_ = NULL;
  g_sound_system = NULL;
  DeleteCriticalSection(&lock_);
  if (primary_ != NULL) {
    primary_->Stop();
    primary_->Release();
    primary_ = NULL;
  }
  if (dsound_ != NULL) {
    dsound_->Release();
    dsound_ = NULL;
  }
}

// 0x4098e0
// TODO: nonmatching: the original increments the pause count before masking
// the flags (maybe a bitfield).
void SoundSystem::Pause() {
  if (state_ & kPaused) {
    return;
  }
  EnterCriticalSection(&lock_);
  thread_.Suspend();
  for (Channel* channel = head_; channel != NULL; channel = channel->next_) {
    unsigned int count = channel->pause_count();
    if (count == 0 && channel->buffer_ != NULL) {
      channel->buffer_->Stop();
    }
    ++count;
    channel->set_pause_count(count);
  }
  for (CSndBuf* buffer = buffers_.tail_; buffer != NULL;
       buffer = buffer->prev_) {
    buffer->Suspend();
  }
  if (primary_ != NULL) {
    primary_->Stop();
  }
  LeaveCriticalSection(&lock_);
  state_ |= kPaused;
}

// 0x409980
void SoundSystem::Resume() {
  if (!(state_ & kPaused)) {
    return;
  }
  EnterCriticalSection(&lock_);
  CreateMixer();
  for (CSndBuf* buffer = buffers_.head_; buffer != NULL;
       buffer = buffer->next_) {
    buffer->Restore();
  }
  for (Channel* channel = head_; channel != NULL; channel = channel->next_) {
    unsigned int count = channel->pause_count();
    if (count != 0 && --count == 0 && !channel->Play()) {
      continue;
    }
    channel->set_pause_count(count);
  }
  thread_.time_ = GetTime();
  thread_.Resume();
  LeaveCriticalSection(&lock_);
  state_ &= ~kPaused;
}

// 0x409a50
void SoundSystem::MuteGroup(int mute, int group) {
  EnterCriticalSection(&lock_);
  Channel* channel = head_;
  if (group >= 0) {
    if (mute) {
      muted_groups_ |= 1 << group;
    } else {
      muted_groups_ &= ~(1 << group);
    }
    for (; channel != NULL; channel = channel->next_) {
      if (channel->sound_->group() == group && channel->buffer_ != NULL) {
        if (mute) {
          channel->buffer_->Stop();
        } else if (channel->flags_ & 0x1) {  // bit 0: meaning unknown
          channel->Play();
        }
      }
    }
  } else {
    muted_groups_ = mute ? 0xffffffff : 0;
    for (; channel != NULL; channel = channel->next_) {
      if (channel->buffer_ != NULL) {
        if (mute) {
          channel->buffer_->Stop();
        } else if (channel->flags_ & 0x1) {
          channel->Play();
        }
      }
    }
  }
  LeaveCriticalSection(&lock_);
}

// 0x408cb0
int SoundSystem::CreateMixer() {
  if (dsound_ != NULL) {
    if (primary_ != NULL) {
      switch (primary_->Play(0, 0, DSBPLAY_LOOPING)) {
        case DS_OK:
          return 1;
        case DSERR_BUFFERLOST:
          primary_->Restore();
          break;
        default:
          primary_->Release();
          primary_ = NULL;
          break;
      }
    }
    if (primary_ == NULL) {
      DSBUFFERDESC desc;
      ZeroMemory(&desc, sizeof(desc));
      desc.dwSize = sizeof(desc);
      desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
      desc.dwBufferBytes = format_.nAvgBytesPerSec;
      desc.lpwfxFormat = &format_;
      desc.guid3DAlgorithm = GUID_NULL;
      if (dsound_->CreateSoundBuffer(&desc, &primary_, NULL) != DS_OK) {
        // "could not create the DirectSoundBuffer"
        Error(
            "DirectSoundBuffer\202\252\220\266\220\254\202\305\202\253\202\334"
            "\202\271\202\361");
        return 0;
      }
    }
    void* data1;
    DWORD size1;
    void* data2;
    DWORD size2;
    if (primary_->Lock(0, format_.nAvgBytesPerSec, &data1, &size1, &data2,
                       &size2, 0) != DS_OK) {
      // "could not lock the sound buffer"
      Error(
          "\273\263\335\304\336\312\336\257\314\247\202\360\333\257\270\202\305"
          "\202\253\202\334\202\271\202\361");
      return 0;
    }
    memset(data1, 0, size1);
    primary_->Unlock(data1, size1, data2, size2);
  }
  return 1;
}
