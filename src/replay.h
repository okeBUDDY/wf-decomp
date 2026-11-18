// Replay: records the player's buttons of a game to a file or plays them
// back (g_replay). The file starts with a header (Begin(), LoadHeader()),
// then holds runs of (buttons, frames).
#ifndef WF_REPLAY_H_
#define WF_REPLAY_H_

#include "file.h"
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
  // Closes the file and switches to `mode` on `path` (opened later).
  int Open(int mode, const char* path);  // 0x415720

 private:
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
