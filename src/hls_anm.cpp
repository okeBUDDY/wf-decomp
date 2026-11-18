// The mesh and sprite animation unit.
#include "hls_anm.h"

#include <stdlib.h>

#include "application.h"
#include "hl_3d.h"
#include "hl_mesh.h"

HL3D* HLMesh::d3d_;  // 0x468eac

// 0x407210
HLMesh::HLMesh() {
  buffer_ = NULL;
  locked_ = NULL;
  vertices_ = NULL;
  unknown_14_ = 0;
  unknown_18_ = unknown_1c_ = unknown_1a_ = unknown_1e_ = unknown_24_ = 0;
  unknown_20_ = 0;
  name_ = "  HLMesh";
  id_ = -1;
}

// 0x407260
void HLMesh::Release() {
  if (buffer_ == NULL) {
    return;
  }
  if (locked_ != NULL) {
    buffer_->Unlock();
  }
  locked_ = NULL;
  if (vertices_ != NULL) {
    free(vertices_);
  }
  vertices_ = NULL;
  buffer_->Release();
  buffer_ = NULL;
  // "[%d] mesh deleted"
  Log("[%d]ﾒｯｼｭ削除", id_);
  if (id_ != -1) {
    d3d_->meshes_[id_] = NULL;
  }
  id_ = -1;
  unknown_14_ = 0;
  unknown_20_ = 0;
  unknown_18_ = unknown_1c_ = unknown_1a_ = unknown_1e_ = unknown_24_ = 0;
}

// 0x4072f0
void HLSAnmSys::Finalize() {
  if (anims_ == NULL) {
    return;
  }
  for (int i = 0; i < count_; ++i) {
    anims_[i].Reset();
  }
  delete[] anims_;
  anims_ = NULL;
}

// 0x4073b0
void HLSAnm::Reset() { cursor_ = NULL; }

// 0x4073c0
void HLSAnm::Hit(HLSHitImage* image, int x, int y, DWORD id) {
  if (object_ != NULL) {
    object_->Hit(image, x, y, id);
  }
}

// 0x4073e0
void HLSAnm::Draw(const Transform* transform, D3DCOLOR color, int blend) {
  if (object_ != NULL) {
    object_->Draw(transform, color, blend);
  }
  if (g_time_moves) {
    Step();
  }
}

// 0x407420
void HLSAnm::Step() {
  if (wait_ <= 0) {
    return;
  }
  if (--wait_ > 0) {
    return;
  }
  cursor_ += 2;
  int command;
  while ((command = cursor_[0]) == kJump) {
    switch (loop_) {
      case -1:
        break;
      case 0:
        loop_ = cursor_[2];
        break;
      case 1:
        loop_ = 0;
        cursor_ += 3;
        continue;
      default:
        --loop_;
        break;
    }
    if (cursor_[1] >= 0) {
      cursor_ = data_->script + cursor_[1];
    } else {
      cursor_ -= cursor_[1];
    }
  }
  if (command == kEnd) {
    return;
  }
  HLSObject** objects = data_->set->objects;
  object_ = objects != NULL ? objects[cursor_[0]] : NULL;
  wait_ = cursor_[1];
}

// 0x4074f0
HLSAnmSys::HLSAnmSys() {
  app_ = NULL;
  anims_ = NULL;
  name_ = "HLSAnmSys";
  count_ = 0x200;
}

// 0x407520
int HLSAnmSys::Initialize(Application* app) {
  app_ = app;
  anims_ = new HLSAnm[count_];
  if (anims_ == NULL) {
    // "Out of memory"
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  // "%d animation objects available."
  Log("%d個のｱﾆﾒｵﾌﾞｼﾞｪｸﾄを利用可能.", count_);
  return 1;
}
