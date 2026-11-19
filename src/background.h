// BackgroundGrid: the scrolling grid behind a game (vtable 0x452878), a
// task fnGame creates. The grid is one tile of 10 x 10 squares, drawn four
// times around the scroll position so that it wraps.
#ifndef WF_BACKGROUND_H_
#define WF_BACKGROUND_H_

#include <d3d.h>

#include "application.h"
#include "hl_3d.h"
#include "task_list.h"

// Turns `transform` by `angle` (1024 steps a turn).
// 0x415f30 (out of line where first used: background.cpp)
inline void RotateTransform(Transform* transform, int angle) {
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

class BackgroundGrid : public Task {
 public:
  enum {
    kSquares = 10,
    kSize = 96,                        // of a square
    kHalfTile = kSquares * kSize / 2,  // 480
    kPoints = 2 * 2 * kSquares,        // the ends of the lines
  };

  BackgroundGrid();          // 0x415fe0
  virtual int Initialize();  // 0x415b90
  virtual void Finalize();   // 0x416010
  // Turns the grid by the pad (every fourth frame) and scrolls it; returns
  // 1.
  virtual int IsProtected();  // 0x415e50
  virtual void Run();         // 0x416030, draws the grid

  float speed_;       // 0x14
  D3DCOLOR color_;    // 0x18
  Application* app_;  // 0x1c

 private:
  // Draws the copy of the grid at `tile`.
  __forceinline void DrawTile(const D3DVECTOR& tile) {
    float x = tile.x;
    float y = tile.y;
    Transform transform;
    transform.m[2][2] = 1;
    transform.m[1][1] = 1;
    transform.m[2][0] = x;
    transform.m[0][0] = 1;
    transform.m[1][2] = 0;
    transform.m[1][0] = 0;
    transform.m[0][2] = 0;
    transform.m[0][1] = 0;
    transform.m[2][1] = y;
    TranslateTransform(&transform, -x_, -y_);
    RotateTransform(&transform, -angle_);
    transform.m[2][0] += 320;
    transform.m[2][1] += 240;
    app_->d3d->DrawPoints(D3DPT_LINELIST, &transform, points_, color_, kPoints);
  }

  D3DVECTOR tiles_[4];  // 0x20, the offsets of the copies
  D3DVECTOR* points_;   // 0x50, kPoints
  short angle_;         // 0x54, of the scrolling
  short unknown_56_;    // 0x56
  float x_;             // 0x58, scroll position
  float y_;             // 0x5c
};

#endif  // WF_BACKGROUND_H_
