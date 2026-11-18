// ScoreTable: the eight best scores (vtable 0x4514a8, global g_score_table).
#ifndef WF_SCORE_TABLE_H_
#define WF_SCORE_TABLE_H_

#include <windows.h>

#include "game_rules.h"

struct ScoreEntry {
  unsigned int score;
  unsigned int time;  // seconds
  short stage;
  short deaths;
  char name[4];
};

class ScoreTable {
 public:
  enum { kEntries = 8 };

  ScoreTable();  // 0x420f60
  // scalar deleting dtor 0x401a30
  virtual ~ScoreTable() {}

  enum { kFileVersion = 0x67 };

  // Selects the rule set whose scores the table holds.
  void SetRules(const GameRules* rules);  // 0x420f80
  // Fills the table with the default scores.
  void Reset();  // 0x420fa0
  // Returns the rank `entry` would get, or -1.
  int FindRank(const ScoreEntry* entry);  // 0x420fe0
  // Inserts `entry` at `rank` (-1: where it belongs); the rank is kept as
  // the highlighted entry.
  void Insert(const ScoreEntry* entry, int rank);  // 0x421010
  // Writes the score list as text into `text` ("@@1"/"@@0" bracket the
  // highlighted line); returns its visible length.
  int Format(char* text);  // 0x4210c0
  // Rewrites the score file with this table only.
  int Save();  // 0x4211e0
  // Tells the player the score file is broken and starts a new one.
  void Recover();  // 0x421330
  // Reads the table for the current rules from the score file.
  int Load();  // 0x421360
  // Stores the table into the score file, adding a record if needed.
  int Store();  // 0x421540
  // Creates the score file from the scores of the old .ini file.
  void Import();  // 0x4217b0
  // Sets the score file path and the default rules.
  void Init(const char* path);  // 0x421990
  // The rank of the last inserted score, -1 for none.
  int rank() const { return rank_; }
  void ClearRank() { rank_ = -1; }

 private:
  // The score file: kFileVersion, a FileHeader, then `count` records
  // of `size` bytes, each a GameRules followed by the entries.
  struct FileHeader {
    int count;
    int size;
  };

  GameRules rules_;               // 0x04
  ScoreEntry entries_[kEntries];  // 0x0c
  int rank_;                      // 0x8c, last inserted rank or -1
  char path_[MAX_PATH];           // 0x90
};

extern ScoreTable g_score_table;  // 0x467620
// "1ST" .. "8TH".
extern const char* kRankNames[ScoreTable::kEntries];  // 0x45de80

#endif  // WF_SCORE_TABLE_H_
