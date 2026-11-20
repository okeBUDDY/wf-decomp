// Replay: records the player's buttons of a game to a file or plays them
// back (g_replay). The file starts with a header (Begin(), LoadHeader()),
// then holds runs of (buttons, frames).
#ifndef WF_REPLAY_H_
#define WF_REPLAY_H_

#include "file.h"
#include "game_rules.h"
#include "object.h"

class Actor;

class Replay : public Object {
 public:
  enum Modes { kNone, kRecord, kPlay };
  enum Flags { kOpen = 0x1 };

  Replay();  // 0x415a90
  // 0x402290, scalar deleting dtor 0x4022c0
  virtual ~Replay() {}

  // Restores the player's state from the file while playing back; 0 when
  // it cannot be read.
  int LoadPlayer(Actor* player);  // 0x415540
  // Writes the last run (while recording) and closes the file.
  void Close();  // 0x415590
  // The buttons held and pressed this frame: the pad's, which are recorded,
  // or the file's.
  void GetInput(unsigned short* keys, unsigned short* pressed);  // 0x4155c0
  // While playing back, opens the file and restores the game from its
  // header; 0 when it cannot be read.
  int LoadHeader();  // 0x415790
  // Starts recording (writing the header and `player`'s state) or playing
  // back.
  void Start(Actor* player);  // 0x415910
  // Closes the file and switches to `mode` on `path` (opened later).
  int Open(int mode, const char* path);  // 0x415720
  // Returns 1 when `path` is a replay file of a version this build plays.
  int IsReplayFile(const char* path);  // 0x415ac0

  int mode() const { return mode_; }
  const char* path() const { return path_; }

 private:
  enum { kVersion = 103, kOldVersion = 101 };

  // The header after the version.
  struct Header {
    int seed;
    int stage;
    int score;
    int timer;
    // Version 103 only (kOldVersion stops here).
    unsigned short lives;
    unsigned short extend_count;
    GameRules rules;
  };

  // One run of the file.
  struct Run {
    unsigned short keys;
    unsigned short frames;
  };

  File file_;                 // 0x08
  Run run_;                   // 0x10
  int mode_;                  // 0x14, Modes
  unsigned int flags_;        // 0x18, Flags
  int seed_;                  // 0x1c, of rand()
  int frame_;                 // 0x20
  unsigned short last_keys_;  // 0x24
  char path_[0x48];           // 0x26
};

extern Replay g_replay;  // 0x467b10

#endif  // WF_REPLAY_H_
