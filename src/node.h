// Node: a transform node in a tree (vtable 0x451538). The destructor is the
// implicit one (folded into ~Object by /OPT:ICF); TaskBase adds the one that
// unlinks the node.
#ifndef WF_NODE_H_
#define WF_NODE_H_

#include "object.h"

class HL3D;

struct Vector3 {
  float x;
  float y;
  float z;
};

class Node : public Object {
 public:
  Node();  // 0x402700

  // Deletes all children, then takes this node out of its parent's list.
  void Unlink();                   // 0x4027a0
  virtual void Detach();           // 0x402850
  virtual void SetMode(int mode);  // 0x402770

  // Set by HL3D::Initialize().
  static HL3D* d3d_;  // 0x468eb0

  void SetScale(float x, float y, float z) {
    scale_.x = x;
    scale_.y = y;
    scale_.z = z;
  }

 protected:
  Vector3 position_;             // 0x08
  Vector3 rotation_;             // 0x14
  Vector3 scale_;                // 0x20
  char unused_2c_[0xac - 0x2c];  // never accessed
  int mode_;                     // 0xac, meaning unknown
  unsigned char flag_b0_;        // 0xb0, meaning unknown
  char unused_b1_[0xcf - 0xb1];  // never accessed
  unsigned char
      visible_;        // 0xcf, set to 1 by the constructor (name unverified)
  Node* parent_;       // 0xd0
  Node* first_child_;  // 0xd4
  Node* last_child_;   // 0xd8
  Node* prev_;         // 0xdc
  Node* next_;         // 0xe0
};

#endif  // WF_NODE_H_
