#include "snd_sys.h"

#include "snd_buf.h"
#include "snd_strm.h"

SoundSystem* Channel::system_;

// 0x40f4c0
void Channel::SetPan(float pan) {
  pan_ = pan;
  if (buffer_ != NULL) {
    if (pan <= -1.0f) {
      buffer_->SetPan(DSBPAN_LEFT);
    } else if (pan >= 1.0f) {
      buffer_->SetPan(DSBPAN_RIGHT);
    } else {
      buffer_->SetPan(static_cast<long>(pan * 5000.0f));
    }
  }
}

// 0x40f530
void Channel::SetVolumeLocked(float volume) {
  Lock();
  if (sound_ != NULL) {
    SetVolume(volume);
  }
  Unlock();
}

// 0x40f570
void Channel::Fade(float from, float to, float time, int stop) {
  if (from < 0.0f) {
    from = volume_;
  }
  if (to < 0.0f) {
    to = volume_;
  }
  if (time <= 0.0f) {
    if (stop) {
      Stop();
    } else {
      SetVolumeLocked(to);
    }
    return;
  }
  Lock();
  if (sound_ != NULL) {
    SetVolume(from);
    if (stop) {
      flags_ |= kFading | kStopAfterFade;
    } else {
      flags_ |= kFading;
    }
    fade_time_ = time;
    fade_elapsed_ = 0.0f;
    fade_from_ = from;
    fade_delta_ = to - from;
  }
  Unlock();
}

// 0x40f650
int Channel::Start() {
  buffer_ = sound_->GetBuffer();
  SetVolume(volume_);
  SetPan(pan_);
  if (buffer_ != NULL) {
    buffer_->SetCurrentPosition(0);
    return Play();
  }
  return 1;
}

// 0x40f6a0
void Channel::Update(float elapsed) {
  if (flags_ & kStream) {
    if (!static_cast<CSndStrm*>(sound_)->Update()) {
      Release();
      return;
    }
    if (flags_ & kStartPending) {
      if (!static_cast<CSndStrm*>(sound_)->ready()) {
        return;
      }
      if (!Start()) {
        Release();
        return;
      }
      flags_ &= ~kStartPending;
    }
  }
  unsigned int flags = flags_;
  if (flags & kJustStarted) {
    flags_ = flags & ~kJustStarted;
    return;
  }
  if (remaining_ > 0.0f) {
    remaining_ -= elapsed;
    if (remaining_ < 0.0f) {
      Release();
      return;
    }
  }
  if (flags & kFading) {
    fade_elapsed_ += elapsed;
    if (fade_elapsed_ >= fade_time_) {
      SetVolume(fade_from_ + fade_delta_);
      if (flags_ & kStopAfterFade) {
        Release();
        return;
      }
      flags_ &= ~kFading;
    } else {
      SetVolume(fade_delta_ / fade_time_ * fade_elapsed_ + fade_from_);
    }
  }
}

// 0x40f7a0
int Channel::Assign(CSndBuf* sound, unsigned int flags, float volume) {
  if (sound == NULL || !(sound->flags_ & CSndBuf::kLoaded)) {
    return 0;
  }
  Lock();
  if (sound_ != NULL) {
    Release();
  }
  sound_ = sound;
  remaining_ = (flags & kLoop) ? -1.0f : sound->seconds_;
  volume_ = volume;
  pan_ = 0;
  flags_ = (flags & kUserFlags) | kJustStarted;
  SoundSystem* system = system_;
  next_ = NULL;
  prev_ = system->tail_;
  if (system->head_ != NULL) {
    system->tail_->next_ = this;
  } else {
    system->head_ = this;
  }
  system->tail_ = this;
  int result;
  if (sound_->GetName() == "CSndStrm") {
    static_cast<CSndStrm*>(sound_)->SetLoop(flags_ & kLoop);
    flags_ |= kStream | kStartPending;
    result = 1;
  } else {
    result = Start();
  }
  Unlock();
  return result;
}
