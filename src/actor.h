// Actor: one object in the ActorSystem pool (0x120 bytes each). The pool
// keeps the actors in place; an actor is in use while `flags` is non-zero.
// Its behaviour comes from the ActorControl registered under `id`.
#ifndef WF_ACTOR_H_
#define WF_ACTOR_H_

#include <windows.h>

class ActorControl;
class HLSObject;

// A D3DCOLOR split into its bytes.
struct Color {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
};

class Actor {
 public:
  // Sets color to `num` / `den` of the way from `from` to `to`.
  void LerpColor(DWORD from, DWORD to, int num, int den);  // 0x407900

  char unknown_00_[0x04];      // 0x00
  unsigned int flags;          // 0x04, 0 when the slot is free
  char unknown_08_[0x0a];      // 0x08
  unsigned short age;          // 0x12, frames since creation
  char unknown_14_[0x04];      // 0x14
  unsigned short angle;        // 0x18
  unsigned short start_angle;  // 0x1a
  char unknown_1c_[0x0c];      // 0x1c
  HLSObject* sprite;           // 0x28
  Color color;                 // 0x2c
  char unknown_30_[0x40];      // 0x30, work area of the control
  ActorControl* control;       // 0x70
  char unknown_74_[0x02];      // 0x74
  short id;                    // 0x76
  short unknown_78_;           // 0x78
  unsigned short unknown_7a_;  // 0x7a
  float speed;                 // 0x7c
  Actor* parent;               // 0x80
  Actor* next;                 // 0x84, next sibling
  Actor* prev;                 // 0x88, previous sibling
  Actor* last_child;           // 0x8c
  Actor* first_child;          // 0x90
  char unknown_94_[0x8c];      // 0x94
};

#endif  // WF_ACTOR_H_
