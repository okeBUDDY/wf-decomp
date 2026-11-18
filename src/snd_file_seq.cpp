#include "snd_file_seq.h"

#include <ctype.h>
#include <string.h>

#include "path_util.h"

namespace {

// Header of a .seq file, after the 4-byte version.
struct SeqHeader {
  unsigned short command_count;
  unsigned short names_size;
  unsigned short max_name_length;
  unsigned short reserved;
};

const int kSeqVersion = 100;

}  // namespace

// 0x40ea60
CSndFileSEQ::CSndFileSEQ()
    : commands_(NULL), names_(NULL), path_(NULL), piece_(NULL) {}

// 0x40ea90
void CSndFileSEQ::Close() {
  ClosePiece();
  if (commands_ != NULL) {
    delete commands_;
    commands_ = NULL;
  }
  if (names_ != NULL) {
    delete names_;
    names_ = NULL;
  }
  if (path_ != NULL) {
    delete path_;
    path_ = NULL;
  }
  if (file_ != NULL) {
    file_->Close();
    delete file_;
    file_ = NULL;
  }
}

// 0x40eb10
// TODO: nonmatching: the original stores &piece_format back into the
// `format` argument slot and uses ebx as the zero register.
CSndFile* CSndFileSEQ::OpenPiece(const char* name, WAVEFORMATEX* format) {
  if (name == NULL) {
    return NULL;
  }
  WAVEFORMATEX piece_format;
  if (format == NULL) {
    format = &piece_format;
  }
  CSndFile* piece;
  switch (tolower(name[strlen(name) - 1])) {
    case 'v':
      piece = new CSndFileWAV;
      break;
    case 'g':
      piece = new CSndFileOGG;
      break;
    default:
      return NULL;
  }
  strcpy(path_ + dir_length_, name);
  if (piece == NULL) {
    return NULL;
  }
  if (piece->Open(path_, format, from_disk_)) {
    if (rate_ == 0) {
      rate_ = static_cast<unsigned short>(format->nSamplesPerSec);
      bits_ = static_cast<unsigned char>(format->wBitsPerSample);
      channels_ = static_cast<unsigned char>(format->nChannels);
      return piece;
    }
    if (rate_ == format->nSamplesPerSec && bits_ == format->wBitsPerSample &&
        channels_ == format->nChannels) {
      return piece;
    }
  }
  piece->Close();
  delete piece;
  return NULL;
}

// 0x40f010
void CSndFileSEQ::Seek(int position) {
  pc_ = 0;
  loop_count_ = 0;
  loop_start_ = 0;
  ClosePiece();
  for (piece_ = OpenPiece(Next(), NULL); piece_ != NULL;
       piece_ = OpenPiece(Next(), NULL)) {
    int size = piece_->size();
    if (size <= 0) {
      return;
    }
    if (position < size) {
      piece_->SeekLooped(position);
      return;
    }
    position -= size;
  }
}

// 0x40f0c0
// TODO: nonmatching: `this` and `left` swap esi/edi.
int CSndFileSEQ::Read(void* buffer, int size) {
  if (piece_ == NULL) {
    return 0;
  }
  char* out = static_cast<char*>(buffer);
  int left = size;
  int read = piece_->Read(out, left);
  left -= read;
  while (left > 0) {
    out += read;
    ClosePiece();
    piece_ = OpenPiece(Next(), NULL);
    if (piece_ == NULL) {
      break;
    }
    read = piece_->Read(out, left);
    left -= read;
  }
  return size - left;
}

// 0x40f1b0
// TODO: nonmatching: register allocation (the original keeps `path` in ebp
// and reloads `from_disk` from the stack).
int CSndFileSEQ::Open(const char* path, WAVEFORMATEX* format, int from_disk) {
  int version;
  SeqHeader header;
  file_ = from_disk ? new File : new PackFile;
  if (file_ != NULL && file_->Open(path, File::kRead) &&
      file_->ReadExact(&version, 4) && version == kSeqVersion &&
      file_->ReadExact(&header, sizeof(header))) {
    command_count_ = header.command_count;
    dir_length_ = static_cast<unsigned short>(GetFileName(path) - path);
    commands_ = new Command[command_count_];
    names_ = new char[header.names_size];
    path_ = new char[header.max_name_length + dir_length_ + 1];
    if (commands_ != NULL && path_ != NULL && names_ != NULL &&
        file_->ReadExact(commands_, command_count_ * sizeof(Command)) &&
        file_->ReadExact(names_, header.names_size)) {
      strcpy(path_, path);
      file_->Close();
      delete file_;
      file_ = NULL;
      from_disk_ = from_disk != 0;
      reserved_1c_ = 0;
      pc_ = 0;
      loop_count_ = 0;
      loop_start_ = 0;
      rate_ = 0;
      piece_ = OpenPiece(Next(), format);
      if (piece_ != NULL) {
        size_ = piece_->size();
        return 1;
      }
    }
  }
  Close();
  return 0;
}
