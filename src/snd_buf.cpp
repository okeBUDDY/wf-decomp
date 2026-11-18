#include "snd_buf.h"

#include <ctype.h>
#include <string.h>

#include "snd_file.h"
#include "snd_file_seq.h"

static char kName[] = "  CSndBuf";  // 0x45d338

// 0x40de80
IDirectSoundBuffer* CSndBuf::GetBuffer() {
  if (buffer_ != NULL) {
    if (flags_ & kBufferInUse) {
      IDirectSoundBuffer* duplicate;
      if (g_sound_system->dsound_->DuplicateSoundBuffer(buffer_,
                                                         &duplicate) != DS_OK) {
        return NULL;
      }
      return duplicate;
    }
    flags_ |= kBufferInUse;
    return buffer_;
  }
  return NULL;
}

// 0x40ded0
void CSndBuf::ReleaseBuffer(IDirectSoundBuffer* buffer) {
  if (buffer == buffer_) {
    flags_ &= ~kBufferInUse;
    return;
  }
  if (buffer != NULL) {
    buffer->Release();
  }
}

// 0x40def0
int CSndBuf::CreateBuffer(WAVEFORMATEX* format, int size) {
  DSBUFFERDESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwSize = sizeof(desc);
  desc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |
                 DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  desc.dwBufferBytes = size;
  desc.lpwfxFormat = format;
  desc.guid3DAlgorithm = GUID_NULL;
  if (g_sound_system->dsound_->CreateSoundBuffer(&desc, &buffer_, NULL) !=
      DS_OK) {
    // "DirectSoundBuffer could not be created"
    Error("DirectSoundBuffer\202\252\220\266\220\254\216\270\224s");
    return 0;
  }
  size_ = size;
  return 1;
}

// 0x40dfd0
void CSndBufList::Remove(CSndBuf* buffer) {
  if (buffer->prev_ != NULL) {
    buffer->prev_->next_ = buffer->next_;
  }
  if (buffer->next_ != NULL) {
    buffer->next_->prev_ = buffer->prev_;
  }
  if (buffer == head_) {
    head_ = buffer->next_;
  }
  if (buffer == tail_) {
    tail_ = buffer->prev_;
  }
  buffer->next_ = NULL;
  buffer->prev_ = NULL;
  buffer->id_ = -1;
}

// 0x40e1d0
// TODO: nonmatching: the original stores the vtable after id_ = -1.
CSndBuf::CSndBuf() {
  id_ = -1;
  next_ = NULL;
  prev_ = NULL;
  buffer_ = NULL;
  path_ = NULL;
  size_ = 0;
  flags_ = 0;
  name_ = kName;
}

// 0x40e230
void CSndBuf::Release() {
  if (flags_ == 0) {
    return;
  }
  Log("[%d]:\312\336\257\314\247\215\355\217\234", id_);  // "buffer deleted"
  g_sound_system->Stop(this, -1);
  g_snd_buf_list->Remove(this);
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

// 0x40e2a0
CSndFile* CSndBuf::OpenSndFile(const char* path, WAVEFORMATEX* format) {
  CSndFile* file;
  switch (tolower(path[strlen(path) - 1])) {
    case 'v':
      file = new CSndFileWAV;
      break;
    case 'q':
      file = new CSndFileSEQ;
      break;
    case 'g':
      file = new CSndFileOGG;
      break;
    default:
      // "unsupported file - %s"
      Error("\273\316\337\260\304\212O\202\314\314\247\262\331 - %s", path);
      return NULL;
  }
  if (file == NULL) {
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return NULL;
  }
  if (!file->Open(path, format, flags_ & kFromDisk)) {
    Error("\265\260\314\337\335\216\270\224s - %s", path);  // "open failed"
    file->Close();
    delete file;
    return NULL;
  }
  return file;
}

// 0x40e420
int CSndBuf::Fill(void* out, CSndFile* file, int offset, int size,
                  int loop) {
  char* dest = static_cast<char*>(out);
  int read = file->Read(dest, size);
  if (read >= size) {
    return 1;
  }
  dest += read;
  size -= read;
  if (loop) {
    while (size > 0) {
      file->Seek(0);
      read = file->Read(dest, size);
      if (read == 0) {
        memset(dest, 0, size);
        return 0;
      }
      size -= read;
      dest += read;
    }
  } else {
    memset(dest, 0, size);
  }
  return 1;
}

// 0x40e4b0
int CSndBuf::Write(CSndFile* file, int offset, int size, int loop) {
  if (size < 0 || offset + size > size_) {
    size = size_ - offset;
  }
  if (size <= 0) {
    return 1;
  }
  void* data1;
  DWORD size1;
  void* data2;
  DWORD size2;
  if (buffer_->Lock(offset, size, &data1, &size1, &data2, &size2, 0) !=
      DS_OK) {
    return 0;
  }
  int result = Fill(data1, file, offset, size, loop);
  buffer_->Unlock(data1, size1, data2, size2);
  return result;
}

// 0x40e560
int CSndBuf::SetPath(const char* path, CSndFile* file, WAVEFORMATEX* format) {
  path_ = new char[strlen(path) + 1];
  if (path_ == NULL) {
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  strcpy(path_, path);
  // "buffer created"
  Log("[%d]:\312\336\257\314\247\220\266\220\254: \"%s\"", id_, path);
  Log("[%d]:%dByte(%dByte) %dch %dHz %dbit %4.2fsec", id_, file->size(),
      size_, format->nChannels, format->nSamplesPerSec,
      format->wBitsPerSample, seconds_);
  return 1;
}

// 0x40e610
// TODO: nonmatching: eax/edx swapped in the two virtual calls of the
// failure path.
int CSndBuf::Load(const char* path, unsigned int flags, unsigned int channel) {
  if (flags_ != 0) {
    Release();
  }
  if (channel >= 4) {
    channel = 0;
  }
  flags_ = (flags & kUserFlags) | channel | kLoaded;
  g_snd_buf_list->Add(this);
  WAVEFORMATEX format;
  CSndFile* file = OpenSndFile(path, &format);
  if (file != NULL) {
    if (g_sound_system->dsound_ != NULL) {
      if (!CreateBuffer(&format, file->size())) {
        goto failed;
      }
      Write(file, 0, -1, 0);
    }
    seconds_ = static_cast<float>(file->size()) / format.nAvgBytesPerSec;
    if (!SetPath(path, file, &format)) {
      goto failed;
    }
    delete file;
    return 1;
  failed:
    delete file;
  }
  Release();
  return 0;
}

// 0x40e720
void CSndBuf::Restore() {
  if (buffer_ == NULL || path_ == NULL) {
    return;
  }
  DWORD status;
  if (buffer_->GetStatus(&status) != DS_OK || status != DSBSTATUS_BUFFERLOST) {
    return;
  }
  buffer_->Restore();
  CSndFile* file = OpenSndFile(path_, NULL);
  if (file != NULL) {
    Write(file, 0, -1, 0);
    delete file;
  }
}
