// Object: root of the game's polymorphic classes.
//
// vtable 0x451270 holds only the scalar deleting destructor. Other classes with
// an empty destructor share ~Object through /OPT:ICF (e.g. 0x406510 calls
// 0x401090).
#ifndef WF_OBJECT_H_
#define WF_OBJECT_H_

class Object {
 public:
  virtual ~Object() {}  // 0x401090, scalar deleting dtor 0x401070
};

#endif  // WF_OBJECT_H_
