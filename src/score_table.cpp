#include "score_table.h"

#include <stdio.h>
#include <string.h>

#include "application.h"
#include "file.h"
#include "game.h"
#include "profile.h"

extern Application g_application;  // 0x467328

static const char* kRankNames[] = {"1ST", "2ND", "3RD", "4TH",
                                   "5TH", "6TH", "7TH", "8TH"};  // 0x45de80
static char s_profile_value[256];                                // 0x468d90

// 0x420f80
void ScoreTable::SetRules(const GameRules* rules) { rules_ = *rules; }

// 0x420fa0
void ScoreTable::Reset() {
  ScoreEntry* entry = entries_;
  unsigned score = 80000;
  for (int i = 0; i < kEntries; ++i, score -= 10000, ++entry) {
    entry->score = score;
    entry->time = 0;
    entry->stage = 1;
    entry->reserved = 0;
    strcpy(entry->name, "---");
  }
  rank_ = -1;
}

// 0x420fe0
int ScoreTable::FindRank(const ScoreEntry* entry) {
  int rank;
  for (rank = kEntries - 1; rank >= 0; --rank) {
    if (entry->score < entries_[rank].score) {
      break;
    }
  }
  ++rank;
  if (rank == kEntries) {
    return -1;
  }
  return rank;
}

// 0x421010
// TODO: nonmatching (rank/entry register allocation swapped)
void ScoreTable::Insert(const ScoreEntry* entry, int rank) {
  if (rank < 0) {
    rank = FindRank(entry);
  }
  if (rank >= 0) {
    for (int i = kEntries - 1; i > rank; --i) {
      entries_[i] = entries_[i - 1];
    }
    entries_[rank] = *entry;
  }
  rank_ = rank;
}

// 0x4210c0
// TODO: nonmatching (register allocation of entry/line)
int ScoreTable::Format(char* text) {
  strcpy(text, "   NAME SCORE      STG DEAD TIME\n\n\n");
  char* line = text + strlen(text);
  for (int i = 0; i < kEntries; ++i) {
    const ScoreEntry* entry = &entries_[i];
    const char* begin;
    const char* end;
    if (i == rank_) {
      begin = "@@1";
      end = "@@0";
    } else {
      end = "";
      begin = "";
    }
    sprintf(line, "%s%s %-3.3s %010d %3d %3d  %02d:%02d:%02d%s\n\n\n", begin,
            kRankNames[i], entry->name, entry->score, entry->stage,
            entry->reserved, entry->time / 3600, entry->time % 3600 / 60,
            entry->time % 60, end);
    line += strlen(line);
  }
  return strlen(text) - (rank_ >= 0 ? 6 : 0);
}

// 0x4211e0
int ScoreTable::Save() {
  File file;
  if (!file.Open(path_, File::kWrite)) {
    return 0;
  }
  int version = kFileVersion;
  if (!file.WriteExact(&version, sizeof(version))) {
    return 0;
  }
  FileHeader header;
  header.count = 1;
  header.size = sizeof(rules_) + sizeof(entries_);
  if (!file.WriteExact(&header, sizeof(header))) {
    return 0;
  }
  if (!file.WriteExact(&rules_, sizeof(rules_))) {
    return 0;
  }
  if (!file.WriteExact(entries_, sizeof(entries_))) {
    return 0;
  }
  return 1;
}

// 0x421330
void ScoreTable::Recover() {
  MessageBox(g_application.window, "Score data was broken.", "WARNING",
             MB_ICONWARNING);
  Reset();
  Save();
}

// 0x421360
// TODO: nonmatching (this/pos register swap, loop alignment)
int ScoreTable::Load() {
  File file;
  if (path_[0] == '\0' || !file.Open(path_, File::kRead)) {
    return 0;
  }
  int version;
  if (!file.ReadExact(&version, sizeof(version))) {
    Recover();
    return 0;
  }
  FileHeader header;
  if (version == kFileVersion && file.ReadExact(&header, sizeof(header))) {
    long pos = file.Tell();
    for (int i = 0; i < header.count; ++i, pos += header.size) {
      file.Seek(pos, SEEK_SET);
      GameRules rules;
      if (!file.ReadExact(&rules, sizeof(rules))) {
        goto broken;
      }
      if (memcmp(&rules, &rules_, sizeof(rules)) == 0) {
        if (!file.ReadExact(entries_, sizeof(entries_))) {
          goto broken;
        }
        return 1;
      }
    }
    return 0;
  }
broken:
  Recover();
  return 0;
}

// 0x421540
// TODO: nonmatching (block layout of the failure returns)
int ScoreTable::Store() {
  File file;
  if (path_[0] == '\0' || !file.Open(path_, File::kReadUpdate)) {
    return 0;
  }
  file.Seek(0, SEEK_SET);
  int version;
  if (!file.ReadExact(&version, sizeof(version))) {
    return 0;
  }
  FileHeader header;
  if (version == kFileVersion && file.ReadExact(&header, sizeof(header))) {
    long pos = file.Tell();
    int i;
    for (i = 0; i < header.count; ++i, pos += header.size) {
      file.Seek(pos, SEEK_SET);
      GameRules rules;
      if (!file.ReadExact(&rules, sizeof(rules))) {
        goto failed;
      }
      if (memcmp(&rules, &rules_, sizeof(rules)) == 0) {
        break;
      }
    }
    file.Seek(pos, SEEK_SET);
    if (!file.WriteExact(&rules_, sizeof(rules_))) {
      return 0;
    }
    if (file.WriteExact(entries_, sizeof(entries_))) {
      if (i != header.count) {
        return 1;
      }
      ++header.count;
      file.Seek(sizeof(version), SEEK_SET);
      if (file.WriteExact(&header, sizeof(header))) {
        return 1;
      }
    }
  }
failed:
  return 0;
}

// 0x4217b0
void ScoreTable::Import() {
  File file;
  if (!file.Open(path_, File::kRead)) {
    Profile::SetSection("Score");
    ScoreEntry* entry = entries_;
    for (int i = 0; i < kEntries; ++i, ++entry) {
      char key[8];
      sprintf(key, "%d", i + 1);
      Profile::GetString(key, "", s_profile_value, sizeof(s_profile_value));
      if (s_profile_value[0] == '\0') {
        Reset();
        break;
      }
      sscanf(s_profile_value, "%d %hd %hd", &entry->score, &entry->stage,
             &entry->reserved);
      sprintf(key, "%dname", i + 1);
      Profile::GetString(key, "---", entry->name, sizeof(entry->name));
      entry->time = 0;
    }
    if (!Save()) {
      path_[0] = '\0';
    }
  }
}

// 0x421990
void ScoreTable::Init(const char* path) {
  g_game.GetDefaultRules(&rules_);
  strcpy(path_, path);
  Import();
  Reset();
}
