// cGlobal: the game itself (g_global, a System named "cGlobal"): it loads
// the data, runs the title, the stages and the results, and keeps the
// state of the current game.
#ifndef WF_CGLOBAL_H_
#define WF_CGLOBAL_H_

#include "score_table.h"
#include "system.h"

class Sprite;

class cGlobal : public System {
 public:
  cGlobal();                                 // 0x4010a0
  virtual int Initialize(Application* app);  // 0x401880
  virtual void Finalize();                   // 0x4010d0
  virtual void Pause() {}
  virtual void Resume() {}

 private:
  // Load the fonts, the pattern files and the sounds; 0 on failure.
  int LoadFonts();     // 0x401280
  int LoadPatterns();  // 0x401470
  int LoadSounds();    // 0x401550
  // Registers one instance of every control in g_actor_system; 0 on
  // failure (actor_control.cpp).
  int RegisterActorControls();  // 0x40ff00

 public:
  enum Flags {
    kFlagStageReplays = 0x1,  // keep a replay of every stage
    kFlagEnglish = 0x4,       // English boss titles
  };

  unsigned int flags;      // 0x08, Flags
  int stage;               // 0x0c, the current stage (from 0)
  int unknown_10_[2];      // 0x10
  ScoreEntry entry;        // 0x18, of the current game
  Sprite* screen_fade;     // 0x28, the overlay that flashes the screen
  float music_volume;      // 0x2c
  float effect_volume;     // 0x30
  char replay_path[0x48];  // 0x34, of the replay to play, "" for none
  char last_record[0x48];  // 0x7c, where the last game was saved

  // Destroys every actor and clears the screen fade.
  void ClearStage();  // 0x4011a0
  // Stops every sound and restores the volumes.
  void ResetSound();  // 0x4011f0
  // Set the volume of the music (sound group 0) and the effects (1).
  void SetMusicVolume(float volume);   // 0x41c6a0
  void SetEffectVolume(float volume);  // 0x41c6c0
  // Clears the state of the game before a new one.
  void ResetGame();  // 0x401240

  // Creates the record directory; returns 1.
  int InitRecords();   // 0x4186f0
  void ClearReplay();  // 0x418710
  // Picks the replay to play next: the last record if it is one, else the
  // first file in rec\ after replay_path (by name), wrapping around. Writes
  // it to `path` and replay_path; 0 when there is none.
  int NextReplay(char* path);  // 0x418720
  // Keeps the replay just recorded (rec\temp.rec): as the boss's replay
  // for the lone titles, and when `finished`, as the best (on a new high
  // score) and the last one; then deletes it.
  void SaveReplay(int finished);  // 0x418a00
};

extern cGlobal g_global;  // 0x467538

// The rating of the last game.
struct GameRating {
  char stage;  // where the game ended
  char grade;  // 0..6
  char title;  // with grades 5 and 6: the first word of the boss's title
};

extern GameRating g_rating;  // 0x468fa0

#endif  // WF_CGLOBAL_H_
