#include "snd_strm.h"

#include "snd_file.h"
#include "snd_file_seq.h"
#include "snd_sys.h"

static char kName[] = "  CSndStrm";  // 0x45d354

// 0x40f8a0
IDirectSoundBuffer* CSndStrm::GetBuffer() {
  if (buffer_ != NULL && !(flags_ & kBufferInUse)) {
    flags_ |= kBufferInUse;
    return buffer_;
  }
  return NULL;
}

// 0x40f8c0
int CSndStrm::Update() {
  if (buffer_ != NULL && file_ != NULL) {
    if (flags_ & kPrefill) {
      unsigned int size =
          size_ - position_ < block_size_ ? size_ - position_ : block_size_;
      if (!Write(file_, position_, size, flags_ & kLoop)) {
        return 0;
      }
      position_ += size;
      if (position_ >= static_cast<unsigned int>(size_)) {
        flags_ &= ~kPrefill;
        buffer_->SetCurrentPosition(0);
        position_ = 0;
      }
    } else {
      DWORD play;
      buffer_->GetCurrentPosition(&play, NULL);
      if (play > position_) {
        if (!Write(file_, position_, play - position_, flags_ & kLoop)) {
          return 0;
        }
        position_ = play;
        return 1;
      }
      if (play < position_) {
        if (!Write(file_, position_, size_ - position_, flags_ & kLoop)) {
          return 0;
        }
        position_ = 0;
      }
    }
  }
  return 1;
}

// 0x40f9b0
CSndStrm::CSndStrm() {
  name_ = kName;
  file_ = NULL;
}

// 0x40f9e0
int CSndStrm::Load(const char* path, unsigned int flags, unsigned int group) {
  if (flags_ != 0) {
    Release();
  }
  if (group >= 4) {
    group = 0;
  }
  flags_ = (flags & kUserFlags) | group | kLoaded;
  g_snd_buf_list->Add(this);
  WAVEFORMATEX format;
  file_ = OpenSndFile(path, &format);
  if (file_ == NULL) {
    goto failed;
  }
  if (g_sound_system->dsound_ != NULL) {
    if (!CreateBuffer(&format, static_cast<int>(
                                   static_cast<float>(format.nSamplesPerSec)) *
                                   format.nBlockAlign)) {
      goto failed;
    }
  }
  seconds_ = static_cast<float>(file_->size()) / format.nAvgBytesPerSec;
  block_size_ = (format.nSamplesPerSec + 15) / 16 * format.nBlockAlign;
  if (!SetPath(path, file_, &format)) {
  failed:
    Release();
    return 0;
  }
  return 1;
}

// 0x40fb00
void CSndStrm::Release() {
  if (flags_ == 0) {
    return;
  }
  Log("[%d]:ﾊﾞｯﾌｧ削除", id_);  // "buffer deleted"
  g_sound_system->Stop(this, -1);
  g_snd_buf_list->Remove(this);
  if (file_ != NULL) {
    file_->Close();
    delete file_;
    file_ = NULL;
  }
  if (buffer_ != NULL) {
    buffer_->Release();
    buffer_ = NULL;
  }
  if (path_ != NULL) {
    delete path_;
    path_ = NULL;
  }
  size_ = 0;
  flags_ = 0;
}

// 0x40fb90
void CSndStrm::Restore() {
  if (buffer_ == NULL || path_ == NULL) {
    return;
  }
  DWORD status;
  if (buffer_->GetStatus(&status) != DS_OK || status != DSBSTATUS_BUFFERLOST) {
    return;
  }
  buffer_->Restore();
  if (flags_ & kBufferInUse) {
    file_->Seek(0);
    if (!(flags_ & kPrefill)) {
      Write(file_, 0, size_, flags_ & kLoop);
      buffer_->SetCurrentPosition(0);
    }
    position_ = 0;
  }
}

// 0x40fc10
const char* CSndStrm::GetPath() {
  if (file_ != NULL && file_->GetName() == "CSndFileSEQ") {
    CSndFileSEQ* seq = static_cast<CSndFileSEQ*>(file_);
    return seq->path() != NULL ? seq->path() : "";
  }
  return CSndBuf::GetPath();
}

// 0x40fc50
void CSndStrm::SetLoop(int loop) {
  if (loop) {
    flags_ |= kLoop;
  } else {
    flags_ &= ~kLoop;
  }
  if (buffer_ != NULL && file_ != NULL) {
    file_->Seek(0);
    flags_ |= kPrefill;
    position_ = 0;
  }
}
