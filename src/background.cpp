#include "background.h"

#include <stdlib.h>

#include "application.h"
#include "hl_3d.h"
#include "hl_input.h"

// 0x415fe0
BackgroundGrid::BackgroundGrid() {
  app_ = NULL;
  points_ = NULL;
  enabled_ = 1;
}

// 0x415b90
int BackgroundGrid::Initialize() {
  if (app_ == NULL) {
    return 0;
  }
  points_ = static_cast<D3DVECTOR*>(malloc(kPoints * sizeof(D3DVECTOR)));
  if (points_ == NULL) {
    Error("\xd2\xd3\xd8\x82\xaa\x91\xab\x82\xe8\x82\xdc\x82\xb9\x82\xf1.");
    return 0;
  }
  D3DVECTOR* p = points_;
  for (int i = 0; i < kSquares; ++i) {
    p->x = -kHalfTile;
    p->y = static_cast<float>(i * kSize - kHalfTile);
    p->z = 0;
    ++p;
    p->x = kHalfTile;
    p->y = static_cast<float>(i * kSize - kHalfTile);
    p->z = 0;
    ++p;
  }
  for (int i = 0; i < kSquares; ++i) {
    p->x = static_cast<float>(i * kSize - kHalfTile);
    p->y = -kHalfTile;
    p->z = 0;
    ++p;
    p->x = static_cast<float>(i * kSize - kHalfTile);
    p->y = kHalfTile;
    p->z = 0;
    ++p;
  }
  tiles_[0].x = tiles_[2].x = -kHalfTile;
  tiles_[1].x = tiles_[3].x = kHalfTile;
  tiles_[0].y = tiles_[1].y = -kHalfTile;
  tiles_[2].y = tiles_[3].y = kHalfTile;
  tiles_[0].z = tiles_[1].z = tiles_[2].z = tiles_[3].z = 0.999999f;
  x_ = y_ = 0;
  angle_ = rand() & 0x3ff;
  unknown_56_ = 0;
  return 1;
}

// 0x416010
void BackgroundGrid::Finalize() {
  if (points_ != NULL) {
    free(points_);
  }
  points_ = NULL;
}

// 0x415e50
int BackgroundGrid::IsProtected() {
  if (!(app_->frame_count & 3)) {
    unsigned short buttons = app_->input->pad(0)->buttons();
    if (buttons & HLInpDev::kRight) {
      ++angle_;
    }
    if (buttons & HLInpDev::kLeft) {
      --angle_;
    }
  }
  x_ += g_sin_table[angle_ & 0x3ff] * speed_;
  y_ -= g_sin_table[(angle_ + 0x100) & 0x3ff] * speed_;
  if (x_ < -kHalfTile) {
    x_ += 2 * kHalfTile;
  }
  if (x_ > kHalfTile) {
    x_ -= 2 * kHalfTile;
  }
  if (y_ < -kHalfTile) {
    y_ += 2 * kHalfTile;
  }
  if (y_ > kHalfTile) {
    y_ -= 2 * kHalfTile;
  }
  return 1;
}

// 0x415f30
void RotateTransform(Transform* transform, int angle) {
  Transform* rotation = reinterpret_cast<Transform*>(g_matrix_work + 0x80);
  rotation->m[0][0] = rotation->m[1][1] = rotation->m[2][2] = 1;
  rotation->m[0][1] = rotation->m[0][2] = rotation->m[1][0] =
      rotation->m[1][2] = rotation->m[2][0] = rotation->m[2][1] = 0;
  reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[0][1] =
      g_sin_table[angle & 0x3ff];
  reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[1][0] =
      -reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[0][1];
  reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[1][1] =
      g_sin_table[(angle + 0x100) & 0x3ff];
  reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[0][0] =
      reinterpret_cast<Transform*>(g_matrix_work + 0x80)->m[1][1];
  MultiplyTransform(
      transform, reinterpret_cast<Transform*>(g_matrix_work + 0x80), transform);
}

// 0x416030
// TODO: nonmatching (the original loads the tile's x and y onto the FPU
// stack before it negates x_ and y_)
void BackgroundGrid::Run() {
  app_->d3d->ClearTexture();
  app_->d3d->SetAlphaBlend(D3DBLEND_INVSRCALPHA);
  for (int i = 0; i < 4; ++i) {
    DrawTile(tiles_[i]);
  }
}
