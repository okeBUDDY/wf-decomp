// Untextured drawing: HL3D::DrawPoints(), the shockwave ring and the
// player's laser.
#include "line_sprites.h"

#include <math.h>

#include "application.h"
#include "hl_3d.h"

// 0x415040
void HL3D::DrawPoints(D3DPRIMITIVETYPE type, const Transform* transform,
                      const D3DVECTOR* points, D3DCOLOR color, int count) {
  if (vertex_count_ + count > kMaxVertices) {
    vertex_count_ = 0;
  }
  D3DTLVERTEX* vertex = &vertices_[vertex_count_];
  for (int i = 0; i < count; ++i) {
    g_vector_work[24] = transform->m[0][0] * points->x +
                        transform->m[1][0] * points->y + transform->m[2][0];
    g_vector_work[25] = transform->m[0][1] * points->x +
                        transform->m[1][1] * points->y + transform->m[2][1];
    vertex->sx = g_vector_work[24];
    vertex->sy = g_vector_work[25];
    vertex->sz = points->z;
    vertex->color = color;
    ++points;
    ++vertex;
  }
  device_->DrawPrimitive(type, D3DFVF_TLVERTEX, &vertices_[vertex_count_],
                         count, 0);
  vertex_count_ += count;
}

// The triangle of the ring.
ShockwaveSprite g_shockwave;  // 0x46904c
LaserSprite g_laser;          // 0x4690f0

static D3DVECTOR g_shockwave_points[3] = {
    {-18.0f, -960.0f, 0.5f},
    {18.0f, -960.0f, 0.5f},
    {-2.0f, 0.0f, 0.5f},
};  // 0x45d5bc

// 0x415120
void ShockwaveSprite::Draw(const Transform* transform, D3DCOLOR color,
                           int blend) {
  if (!(color & 0xff000000)) {
    return;
  }
  g_application.d3d->ClearTexture();
  switch (blend) {
    case 1:
      g_application.d3d->SetAlphaBlend(D3DBLEND_ONE);
      g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, transform,
                                    g_shockwave_points, color, 3);
      g_application.d3d->SetAlphaBlend(D3DBLEND_INVSRCALPHA);
      break;
    default:
      g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, transform,
                                    g_shockwave_points, color, 3);
      break;
  }
}

// 0x415220
void LaserSprite::Reset(float x, float y) {
  tip_x_ = x;
  tip_y_ = y - kMaxLength;
  x_ = x;
  y_ = y;
  angle_ = 0;
  spread_ = 16;
}

// 0x415250
void LaserSprite::Move(float x, float y, int hold, float push_x, float push_y) {
  if (hold) {
    tip_x_ = x + tip_x_ - x_;
    tip_y_ = y + tip_y_ - y_;
  } else {
    tip_x_ += push_x;
    tip_y_ += push_y;
    float dx = tip_x_ - x;
    float dy = tip_y_ - y;
    float length = sqrt(dy * dy + dx * dx);
    float angle = atan2(dx, -dy);
    if (length > kMaxLength) {
      length = kMaxLength;
      tip_x_ = sin(angle) * kMaxLength + x;
      tip_y_ = y - cos(angle) * kMaxLength;
    }
    angle_ = static_cast<short>(angle * 162.97466f);
    spread_ = 160 - static_cast<short>(length * 1.0285715f);
  }
  short left = angle_ + spread_;
  points_[0].x = g_sin_table[left & 0x3ff] * 2048.0f;
  points_[0].y = g_sin_table[(left + 0x100) & 0x3ff] * -2048.0f;
  short right = angle_ - spread_;
  points_[2].x = g_sin_table[right & 0x3ff] * 2048.0f;
  points_[2].y = g_sin_table[(right + 0x100) & 0x3ff] * -2048.0f;
  x_ = x;
  y_ = y;
}

// 0x4153c0
LaserSprite::LaserSprite() {
  tip_x_ = 0.0f;
  tip_y_ = -140.0f;
  x_ = 0.0f;
  y_ = 0.0f;
  angle_ = 0;
  spread_ = 16;
  points_[1].x = 0.0f;
  points_[1].y = 0.0f;
  points_[1].z = 0.5f;
  points_[2].z = 0.5f;
  points_[0].z = 0.5f;
}

// 0x415400
void LaserSprite::Draw(const Transform* transform, D3DCOLOR color, int blend) {
  if (!(color & 0xff000000)) {
    return;
  }
  D3DCOLOR dim = ((color >> 3) & 0x1f000000) | (color & 0xffffff);
  g_application.d3d->ClearTexture();
  switch (blend) {
    case 1:
      g_application.d3d->SetAlphaBlend(D3DBLEND_ONE);
      g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, transform, points_,
                                    dim, 3);
      g_application.d3d->DrawPoints(D3DPT_LINESTRIP, transform, points_, color,
                                    3);
      g_application.d3d->SetAlphaBlend(D3DBLEND_INVSRCALPHA);
      break;
    default:
      g_application.d3d->DrawPoints(D3DPT_TRIANGLESTRIP, transform, points_,
                                    dim, 3);
      g_application.d3d->DrawPoints(D3DPT_LINESTRIP, transform, points_, color,
                                    3);
      break;
  }
}
