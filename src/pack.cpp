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
static char* pack_path;          // 0x468f94

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
