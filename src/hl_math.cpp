// The transforms of HL3D, computed in its scratch buffers.
#include "hl_3d.h"

// 0x402370
void TransformPoint(D3DTLVERTEX* out, const Transform* transform,
                    const D3DVECTOR* point) {
  g_vector_work[0x18] = transform->m[0][0] * point->x +
                        transform->m[1][0] * point->y + transform->m[2][0];
  g_vector_work[0x19] = transform->m[0][1] * point->x +
                        transform->m[1][1] * point->y + transform->m[2][1];
  out->sx = g_vector_work[0x18];
  out->sy = g_vector_work[0x19];
  out->sz = point->z;
}

// The product being built by MultiplyTransform, in g_matrix_work.
static inline float& Product(int row, int column) {
  return g_matrix_work[0x90 + row * 4 + column];
}

// 0x4023d0
void MultiplyTransform(Transform* out, const Transform* a, const Transform* b) {
  Product(0, 0) = b->m[0][0] * a->m[0][0] + b->m[0][1] * a->m[1][0] +
                  b->m[0][2] * a->m[2][0];
  Product(0, 1) = b->m[0][0] * a->m[0][1] + b->m[0][1] * a->m[1][1] +
                  b->m[0][2] * a->m[2][1];
  Product(0, 2) = b->m[0][0] * a->m[0][2] + b->m[0][1] * a->m[1][2] +
                  b->m[0][2] * a->m[2][2];
  Product(1, 0) = b->m[1][0] * a->m[0][0] + b->m[1][1] * a->m[1][0] +
                  b->m[1][2] * a->m[2][0];
  Product(1, 1) = b->m[1][0] * a->m[0][1] + b->m[1][1] * a->m[1][1] +
                  b->m[1][2] * a->m[2][1];
  Product(1, 2) = b->m[1][0] * a->m[0][2] + b->m[1][1] * a->m[1][2] +
                  b->m[1][2] * a->m[2][2];
  Product(2, 0) = b->m[2][0] * a->m[0][0] + b->m[2][1] * a->m[1][0] +
                  b->m[2][2] * a->m[2][0];
  Product(2, 1) = b->m[2][0] * a->m[0][1] + a->m[1][1] * b->m[2][1] +
                  b->m[2][2] * a->m[2][1];
  Product(2, 2) = b->m[2][0] * a->m[0][2] + b->m[2][1] * a->m[1][2] +
                  b->m[2][2] * a->m[2][2];
  Transform* product = reinterpret_cast<Transform*>(&Product(0, 0));
  out->m[0][0] = product->m[0][0];
  out->m[0][1] = product->m[0][1];
  out->m[0][2] = product->m[0][2];
  out->m[1][0] = product->m[1][0];
  out->m[1][1] = product->m[1][1];
  out->m[1][2] = product->m[1][2];
  out->m[2][0] = product->m[2][0];
  out->m[2][1] = product->m[2][1];
  out->m[2][2] = product->m[2][2];
}
