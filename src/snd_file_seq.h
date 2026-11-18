// CSndFileSEQ: a sequence of WAV/OGG pieces played one after another
// (vtable 0x452314).
#ifndef WF_SND_FILE_SEQ_H_
#define WF_SND_FILE_SEQ_H_

#include "snd_file.h"

// The flags the commands of a .seq file branch on. FnGame::RateGame() stores
// the rating of a finished game in the first three (g_rating overlays them)
// and FnGame the state of the game in the others.
extern unsigned char g_seq_flags[8];  // 0x468fa0
// Clears g_seq_flags.
void ClearSeqFlags();  // 0x40ea20

class CSndFileSEQ : public CSndFile {
 public:
  CSndFileSEQ();  // 0x40ea60
  // 0x40f160, scalar deleting dtor 0x40f140
  virtual ~CSndFileSEQ() { Close(); }
  // 0x40ea80
  virtual const char* GetName() { return "CSndFileSEQ"; }
  virtual int Open(const char* path, WAVEFORMATEX* format,
                   int from_disk);           // 0x40f1b0
  virtual void Close();                      // 0x40ea90
  virtual int Read(void* buffer, int size);  // 0x40f0c0
  virtual void Seek(int position);           // 0x40f010

  const char* path() const { return path_; }

 private:
  // The commands of a .seq file.
  enum Op {
    kOpPlay = 0x00,         // play `target` `count` times (0: for ever)
    kOpPlayIfEqual = 0x01,  // the same if variable `var` == `operand`
    kOpPlayIfNotEqual = 0x02,
    kOpPlayIfAtLeast = 0x03,
    kOpPlayIfAtMost = 0x04,
    kOpEnd = 0x10,     // stop
    kOpRepeat = 0x11,  // jump to `target` `count` - 1 more times (0: for ever)
    kOpSet = 0x20,     // `var` = `operand`
    kOpAdd = 0x21,     // `var` += `operand`
    kOpRandom = 0x22,  // `var` = random in [`low`, `high`]
    kOpJump = 0x30,    // go to command `target`
    kOpJumpIfEqual = 0x31,  // the same if variable `var` == `operand`
    kOpJumpIfNotEqual = 0x32,
    kOpJumpIfAtLeast = 0x33,
    kOpJumpIfAtMost = 0x34,
  };
  // Variables 0x00-0x0f are the file's own (vars_), 0x10-0x1f g_seq_flags.
  enum { kGlobalVars = 0x10 };

  struct Command {
    unsigned char op;     // Op
    unsigned char count;  // of kOpPlay and kOpRepeat
    unsigned char var;
    char operand;
    union {
      unsigned short target;  // name offset or command index
      struct {
        char low;
        char high;
      } range;  // of kOpRandom
    };
  };

  void ClosePiece() {
    if (piece_ != NULL) {
      piece_->Close();
      delete piece_;
      piece_ = NULL;
    }
  }
  // Opens piece `name`; it must have the format of the first piece.
  CSndFile* OpenPiece(const char* name, WAVEFORMATEX* format);  // 0x40eb10
  // Runs the commands up to the next piece; returns its name.
  const char* Next();  // 0x40ec40
  // The variable `id`.
  char* Variable(int id) {
    if ((id & 0xf0) == kGlobalVars) {
      return reinterpret_cast<char*>(&g_seq_flags[id & 0xf]);
    }
    return &vars_[id & 0xf];
  }
  // Starts playing the piece of `command`; returns its name.
  __forceinline const char* Play(const Command* command) {
    piece_loops_ = command->count > 0 ? command->count : -1;
    return names_ + command->target;
  }

  unsigned short command_count_;  // 0x0c
  unsigned short pc_;             // 0x0e
  short repeat_count_;         // 0x10, of the kOpRepeat running, -1: for ever
  short piece_loops_;          // 0x12, plays left of the piece, -1: for ever
  short from_disk_;            // 0x14
  unsigned short dir_length_;  // 0x16, length of the directory in path_
  unsigned short rate_;        // 0x18, sample rate of the first piece
  unsigned char bits_;         // 0x1a
  unsigned char channels_;     // 0x1b
  char vars_[4];               // 0x1c
  Command* commands_;          // 0x20
  char* names_;                // 0x24
  char* path_;                 // 0x28
  CSndFile* piece_;            // 0x2c
};

#endif  // WF_SND_FILE_SEQ_H_
