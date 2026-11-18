// The data.bin archive directory (0x40dbb0-0x40de66).
#include <string.h>

#include "file.h"

struct PackEntry {
  char* name;
  long offset;
  long size;
};

static int pack_count;           // 0x468f88
static PackEntry* pack_entries;  // 0x468f8c
static char* pack_names;         // 0x468f90
static char* pack_path;          // 0x468f94

struct PackHeader {
  long count;
  long names_size;
};

// 0x40dbb0. NOT MATCHING yet: register allocation/scheduling of the search
// loop differs slightly (the original keeps the count in ebp from the start).
int PackFile::Open(const char* name, int mode) {
  if (pack_count == 0 || mode != kRead) {
    return 0;
  }
  int left = pack_count;
  PackEntry* entry = pack_entries;
  for (; left > 0; left--, entry++) {
    if (strcmp(name, entry->name) == 0) {
      break;
    }
  }
  if (left <= 0) {
    return 0;
  }
  file_ = fopen(pack_path, "rb");
  if (file_ == NULL) {
    return 0;
  }
  offset_ = entry->offset;
  size_ = entry->size;
  pos_ = 0;
  fseek(file_, offset_, SEEK_SET);
  return 1;
}

// 0x40dc80
void ClosePack() {
  if (pack_entries != NULL) {
    delete pack_entries;
    pack_entries = NULL;
  }
  if (pack_names != NULL) {
    delete pack_names;
    pack_names = NULL;
  }
  if (pack_path != NULL) {
    delete pack_path;
    pack_path = NULL;
  }
  pack_count = 0;
}

// 0x40dce0. NOT MATCHING yet: the original inlines File::Open and the first
// File::Read (so they were probably defined in the class), which this
// File does not reproduce.
int OpenPack(const char* path) {
  if (pack_count != 0) {
    ClosePack();
  }
  File file;
  if (!file.Open(path, File::kRead)) {
    return 0;
  }
  PackHeader header;
  file.Read(&header, sizeof(header));
  if (header.count > 0) {
    pack_count = header.count;
    pack_entries = new PackEntry[pack_count];
    pack_names = new char[header.names_size];
    pack_path = new char[strlen(path) + 1];
    if (pack_entries != NULL && pack_names != NULL && pack_path != NULL) {
      file.Read(pack_entries, pack_count * sizeof(PackEntry));
      file.Read(pack_names, header.names_size);
      strcpy(pack_path, path);
      // The directory stores name offsets into the name block.
      for (int i = 0; i < pack_count; i++) {
        pack_entries[i].name =
            pack_names + reinterpret_cast<long>(pack_entries[i].name);
      }
      return 1;
    }
    ClosePack();
  }
  return 0;
}
