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

// The rotation the RotateTransform*() functions build.
static inline Transform* Rotation() { return &g_matrix_work->rotation; }

// 0x402550
void RotateTransformX(Transform* transform, float radians) {
  SetIdentity(Rotation());
  Rotation()->m[1][2] = static_cast<float>(sin(radians));
  Rotation()->m[2][1] = -Rotation()->m[1][2];
  Rotation()->m[2][2] = static_cast<float>(cos(radians));
  Rotation()->m[1][1] = Rotation()->m[2][2];
  MultiplyTransform(transform, Rotation(), transform);
}

// 0x4025e0
void RotateTransformY(Transform* transform, float radians) {
  SetIdentity(Rotation());
  Rotation()->m[2][0] = static_cast<float>(sin(radians));
  Rotation()->m[0][2] = -Rotation()->m[2][0];
  Rotation()->m[2][2] = static_cast<float>(cos(radians));
  Rotation()->m[0][0] = Rotation()->m[2][2];
  MultiplyTransform(transform, Rotation(), transform);
}

// 0x402670
void RotateTransformZ(Transform* transform, float radians) {
  SetIdentity(Rotation());
  Rotation()->m[0][1] = static_cast<float>(sin(radians));
  Rotation()->m[1][0] = -Rotation()->m[0][1];
  Rotation()->m[1][1] = static_cast<float>(cos(radians));
  Rotation()->m[0][0] = Rotation()->m[1][1];
  MultiplyTransform(transform, Rotation(), transform);
}
