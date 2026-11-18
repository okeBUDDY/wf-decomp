// CSndFileSEQ: a sequence of WAV/OGG pieces played one after another
// (vtable 0x452314).
#ifndef WF_SND_FILE_SEQ_H_
#define WF_SND_FILE_SEQ_H_

#include "snd_file.h"

// The flags the commands of a .seq file branch on. RateGame() stores the
// rating of a finished game in the first three (g_rating overlays them)
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
  struct Command {
    unsigned char op;
    unsigned char arg;
    unsigned short value[2];
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

  unsigned short command_count_;  // 0x0c
  unsigned short pc_;             // 0x0e
  unsigned short loop_start_;     // 0x10
  unsigned short loop_count_;     // 0x12
  short from_disk_;               // 0x14
  unsigned short dir_length_;     // 0x16, length of the directory in path_
  unsigned short rate_;           // 0x18, sample rate of the first piece
  unsigned char bits_;            // 0x1a
  unsigned char channels_;        // 0x1b
  int reserved_1c_;               // 0x1c
  Command* commands_;             // 0x20
  char* names_;                   // 0x24
  char* path_;                    // 0x28
  CSndFile* piece_;               // 0x2c
};

#endif  // WF_SND_FILE_SEQ_H_
