// The transforms of HL3D, computed in its scratch buffers.
#include "hl_3d.h"

#include <math.h>

// 0x402370
void TransformPoint(D3DTLVERTEX* out, const Transform* transform,
                    const D3DVECTOR* point) {
  g_vector_work->point.x = transform->m[0][0] * point->x +
                           transform->m[1][0] * point->y + transform->m[2][0];
  g_vector_work->point.y = transform->m[0][1] * point->x +
                           transform->m[1][1] * point->y + transform->m[2][1];
  out->sx = g_vector_work->point.x;
  out->sy = g_vector_work->point.y;
  out->sz = point->z;
}

// 0x402550
void RotateTransformX(Transform* transform, float radians) {
  SetIdentity(&g_matrix_work->rotation);
  g_matrix_work->rotation.m[1][2] = static_cast<float>(sin(radians));
  g_matrix_work->rotation.m[2][1] = -g_matrix_work->rotation.m[1][2];
  g_matrix_work->rotation.m[2][2] = static_cast<float>(cos(radians));
  g_matrix_work->rotation.m[1][1] = g_matrix_work->rotation.m[2][2];
  MultiplyTransform(transform, &g_matrix_work->rotation, transform);
}

// 0x4025e0
void RotateTransformY(Transform* transform, float radians) {
  SetIdentity(&g_matrix_work->rotation);
  g_matrix_work->rotation.m[2][0] = static_cast<float>(sin(radians));
  g_matrix_work->rotation.m[0][2] = -g_matrix_work->rotation.m[2][0];
  g_matrix_work->rotation.m[2][2] = static_cast<float>(cos(radians));
  g_matrix_work->rotation.m[0][0] = g_matrix_work->rotation.m[2][2];
  MultiplyTransform(transform, &g_matrix_work->rotation, transform);
}

// 0x402670
void RotateTransformZ(Transform* transform, float radians) {
  SetIdentity(&g_matrix_work->rotation);
  g_matrix_work->rotation.m[0][1] = static_cast<float>(sin(radians));
  g_matrix_work->rotation.m[1][0] = -g_matrix_work->rotation.m[0][1];
  g_matrix_work->rotation.m[1][1] = static_cast<float>(cos(radians));
  g_matrix_work->rotation.m[0][0] = g_matrix_work->rotation.m[1][1];
  MultiplyTransform(transform, &g_matrix_work->rotation, transform);
}
