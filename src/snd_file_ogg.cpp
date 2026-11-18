#include "snd_file.h"

// ov_callbacks over a File.

// 0x40e840
static size_t ReadCallback(void* buffer, size_t size, size_t count,
                           void* source) {
  File* file = static_cast<File*>(source);
  return file->Read(buffer, size * count) / size;
}

// 0x40e870
static int SeekCallback(void* source, ogg_int64_t offset, int origin) {
  File* file = static_cast<File*>(source);
  return file->Seek(static_cast<long>(offset), origin);
}

// 0x40e890
static int CloseCallback(void* source) { return 0; }

// 0x40e8a0
static long TellCallback(void* source) {
  File* file = static_cast<File*>(source);
  return file->Tell();
}

static ov_callbacks kCallbacks = {ReadCallback, SeekCallback, CloseCallback,
                                  TellCallback};  // 0x45d344

// 0x40e790
void CSndFileOGG::Seek(int position) {
  if (size_ != 0) {
    ov_pcm_seek(&vorbis_, position / block_align_);
    position_ = position;
  }
}

// 0x40e7d0
int CSndFileOGG::Read(void* buffer, int size) {
  if (size_ == 0) {
    return 0;
  }
  if (size > size_ - position_) {
    size = size_ - position_;
  }
  int left = size;
  while (left > 0) {
    int read =
        ov_read(&vorbis_, static_cast<char*>(buffer), left, 0, 2, 1, NULL);
    if (read <= 0) {
      return size - left;
    }
    left -= read;
    buffer = static_cast<char*>(buffer) + read;
  }
  return size;
}

// 0x40e8b0
void CSndFileOGG::Close() {
  if (size_ != 0) {
    ov_clear(&vorbis_);
    size_ = 0;
  }
  if (file_ != NULL) {
    file_->Close();
    delete file_;
    file_ = NULL;
  }
}

// 0x40e8f0
// TODO: nonmatching: the original places the shared "return 0" block right
// after the ov_info() check instead of at the end (same instructions).
int CSndFileOGG::Open(const char* path, WAVEFORMATEX* format, int from_disk) {
  file_ = from_disk ? new File : new PackFile;
  if (file_ == NULL || !file_->Open(path, File::kRead)) {
    return 0;
  }
  if (ov_open_callbacks(file_, &vorbis_, NULL, 0, kCallbacks) != 0) {
    return 0;
  }
  size_ = 1;  // lets Close() call ov_clear()
  vorbis_info* info = ov_info(&vorbis_, -1);
  if (info == NULL) {
    return 0;
  }
  format->wFormatTag = WAVE_FORMAT_PCM;
  format->nChannels = info->channels;
  format->nSamplesPerSec = info->rate;
  format->wBitsPerSample = 16;
  format->nBlockAlign = info->channels * 2;
  format->nAvgBytesPerSec = format->nBlockAlign * info->rate;
  format->cbSize = sizeof(WAVEFORMATEX);
  block_align_ = format->nBlockAlign;
  size_ = static_cast<int>(ov_pcm_total(&vorbis_, -1) * block_align_);
  position_ = 0;
  return 1;
}
