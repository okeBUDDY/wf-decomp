// HLMesh: a Direct3D vertex buffer registered with HL3D (vtable 0x451fe0).
#ifndef WF_HL_MESH_H_
#define WF_HL_MESH_H_

#include <windows.h>

#include <d3d.h>

#include "object.h"

class HL3D;

class HLMesh : public Object {
 public:
  HLMesh();  // 0x407210
  // Releases the vertex buffer and unregisters the mesh.
  void Release();  // 0x407260

  static HL3D* d3d_;  // 0x468eac

 private:
  IDirect3DVertexBuffer7* buffer_;  // 0x08
  void* locked_;                    // 0x0c, while locked
  void* vertices_;                  // 0x10, malloc()ed
  // 0x14..0x25 (unused_*): only cleared by the constructor and Release(); the
  // code that used them (creating, filling and drawing a buffer) was discarded
  // by /OPT:REF.
  int unused_14_;    // 0x14
  short unused_18_;  // 0x18
  short unused_1a_;  // 0x1a
  short unused_1c_;  // 0x1c
  short unused_1e_;  // 0x1e
  int unused_20_;    // 0x20
  short unused_24_;  // 0x24
  short id_;         // 0x26, index in HL3D, -1: none
};

#endif  // WF_HL_MESH_H_
