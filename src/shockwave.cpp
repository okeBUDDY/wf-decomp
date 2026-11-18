// Untextured drawing: HL3D::DrawPoints() and the shockwave ring.
#include "shockwave.h"

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
