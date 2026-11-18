#include <string.h>

#include "snd_file.h"

// 0x40f390
int CSndFileWAV::Open(const char* path, WAVEFORMATEX* format, int from_disk) {
  file_ = from_disk ? new File : new PackFile;
  if (file_ == NULL || !file_->Open(path, File::kRead)) {
    return 0;
  }
  file_->Seek(12, SEEK_SET);  // skip "RIFF" <size> "WAVE"
  int size;
  char id[4];
  if (file_->Read(id, 4) < 4) {
    return 0;
  }
  do {
    file_->Read(&size, 4);
    if (strncmp(id, "data", 4) == 0) {
      size_ = size;
      data_offset_ = file_->Tell();
      return 1;
    }
    if (strncmp(id, "fmt ", 4) == 0) {
      file_->Read(format, size);
      format->cbSize = sizeof(WAVEFORMATEX);
    } else {
      file_->Seek(size, SEEK_CUR);
    }
  } while (file_->Read(id, 4) >= 4);
  return 0;
}
