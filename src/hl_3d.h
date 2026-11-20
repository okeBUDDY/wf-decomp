// HL3D: the Direct3D 7 device drawn into the back buffer (vtable 0x451568).
#ifndef WF_HL_3D_H_
#define WF_HL_3D_H_

#include <windows.h>

#include <d3d.h>

#include "hl_surface.h"
#include "system.h"

struct Application;
class HLMesh;
class HLSurface;

// A 2D transform in a 4x4 matrix: x' = x * m[0][0] + y * m[1][0] + m[2][0],
// and the same with column 1 for y'.
struct Transform {
  float m[4][4];
};

// A texture coordinate.
struct TexCoord {
  float u;
  float v;
};

// Transforms `point` by `transform` into the screen position of `out`.
void TransformPoint(D3DTLVERTEX* out, const Transform* transform,
                    const D3DVECTOR* point);  // 0x402370

// Makes the 3x3 part of `transform` the identity.
inline void SetIdentity(Transform* transform) {
  transform->m[0][0] = transform->m[1][1] = transform->m[2][2] = 1;
  transform->m[0][1] = transform->m[0][2] = transform->m[1][0] =
      transform->m[1][2] = transform->m[2][0] = transform->m[2][1] = 0;
}

// Scales `transform` by `sx` and `sy`.
inline void ScaleTransform(Transform* transform, float sx, float sy) {
  transform->m[0][0] *= sx;
  transform->m[1][0] *= sx;
  transform->m[2][0] *= sx;
  transform->m[0][1] *= sy;
  transform->m[1][1] *= sy;
  transform->m[2][1] *= sy;
}

// Moves `transform` by (`x`, `y`).
inline void TranslateTransform(Transform* transform, float x, float y) {
  transform->m[2][0] += x;
  transform->m[2][1] += y;
}

// Rotates `transform` by `radians` about the x, y and z axes.
void RotateTransformX(Transform* transform, float radians);  // 0x402550
void RotateTransformY(Transform* transform, float radians);  // 0x4025e0
void RotateTransformZ(Transform* transform, float radians);  // 0x402670

// The scratch vectors of the transforms (g_vector_work). Only the vector
// that TransformPoint() and the Draw*() loops transform into is used.
struct VectorWork {
  D3DVECTOR unused[8];  // 0x00
  D3DVECTOR point;      // 0x60, the point being transformed
  D3DVECTOR unused_6c;  // 0x6c
};

// The scratch matrices of the transforms (g_matrix_work).
struct MatrixWork {
  Transform unused[8];  // 0x000
  Transform rotation;   // 0x200, built by RotateTransform*()
  Transform product;    // 0x240, built by MultiplyTransform()
};

// Math tables and scratch space, allocated by HL3D::CreateBuffers().
extern float* g_sin_table;         // 0x468878, sin of 1024 steps a turn (+256)
extern float* g_angle_table;       // 0x46887c, the radians of each step
extern VectorWork* g_vector_work;  // 0x468870
extern MatrixWork* g_matrix_work;  // 0x468874
extern Transform g_base_matrix;    // 0x468830

// `out` = `a` x `b`. 0x4023d0, out of line where first used (hl_math.cpp).
inline void MultiplyTransform(Transform* out, const Transform* a,
                              const Transform* b) {
  g_matrix_work->product.m[0][0] = b->m[0][0] * a->m[0][0] +
                                   b->m[0][1] * a->m[1][0] +
                                   b->m[0][2] * a->m[2][0];
  g_matrix_work->product.m[0][1] = b->m[0][0] * a->m[0][1] +
                                   b->m[0][1] * a->m[1][1] +
                                   b->m[0][2] * a->m[2][1];
  g_matrix_work->product.m[0][2] = b->m[0][0] * a->m[0][2] +
                                   b->m[0][1] * a->m[1][2] +
                                   b->m[0][2] * a->m[2][2];
  g_matrix_work->product.m[1][0] = b->m[1][0] * a->m[0][0] +
                                   b->m[1][1] * a->m[1][0] +
                                   b->m[1][2] * a->m[2][0];
  g_matrix_work->product.m[1][1] = b->m[1][0] * a->m[0][1] +
                                   b->m[1][1] * a->m[1][1] +
                                   b->m[1][2] * a->m[2][1];
  g_matrix_work->product.m[1][2] = b->m[1][0] * a->m[0][2] +
                                   b->m[1][1] * a->m[1][2] +
                                   b->m[1][2] * a->m[2][2];
  g_matrix_work->product.m[2][0] = b->m[2][0] * a->m[0][0] +
                                   b->m[2][1] * a->m[1][0] +
                                   b->m[2][2] * a->m[2][0];
  g_matrix_work->product.m[2][1] = b->m[2][0] * a->m[0][1] +
                                   a->m[1][1] * b->m[2][1] +
                                   b->m[2][2] * a->m[2][1];
  g_matrix_work->product.m[2][2] = b->m[2][0] * a->m[0][2] +
                                   b->m[2][1] * a->m[1][2] +
                                   b->m[2][2] * a->m[2][2];
  Transform* product = &g_matrix_work->product;
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

class HLCamera;

class HL3D : public System {
 public:
  // flags_: the low word holds HLSurface::texture_caps_.
  enum Flags { kInScene = 0x10000, kTnLDevice = 0x20000 };
  enum { kMaxVertices = 0x800, kMaxMeshes = 128 };

  HL3D();                                    // 0x402bb0
  virtual int Initialize(Application* app);  // 0x4030c0
  virtual void Finalize();                   // 0x402e60
  virtual void Pause() {}
  virtual void Resume();  // 0x402990

  // Starts the frame's scene unless one is open.
  void BeginScene() {
    if (!(flags_ & kInScene) && device_->BeginScene() == D3D_OK) {
      flags_ |= kInScene;
    }
  }
  // Clears the target (and the z-buffer when there is one).
  void Clear() {  // 0x402a20
    device_->Clear(0, NULL,
                   (zbuffer_ != NULL ? D3DCLEAR_ZBUFFER : 0) | D3DCLEAR_TARGET,
                   0, 1.0f, 0);
  }
  // Sets the texture of `stage` unless it is the current one.
  void SetTexture(HLSurface* texture, int stage);  // 0x407880

  // Sets the alpha of the material's diffuse color.
  void SetAlpha(float alpha);  // 0x402a00
  // Turns alpha blending on or off; a zero blend factor keeps the current.
  void SetBlend(BOOL enable, DWORD src, DWORD dest);  // 0x402a50
  // Sets the color and alpha operation of `stage` (and their arguments
  // when they are non-zero).
  void SetTextureOp(DWORD op, DWORD arg1, DWORD arg2, DWORD stage);  // 0x402ac0
  // Chooses linear or point filtering for `stage`.
  void SetFilter(BOOL linear, DWORD stage);  // 0x402b50
  void SetLighting(BOOL enable);             // 0x4030a0
  // The z-buffer bits (before Initialize(); 0: no z-buffer).
  void set_depth(short depth) { depth_ = depth; }
  // The texture coordinate of the texel `point` of `texture`.
  void GetTexCoord(HLSurface* texture, TexCoord* out,
                   const POINTS* point);  // 0x409bb0
  // Transforms `count` points by `transform` and draws them with their
  // texture coordinates in `color`.
  void DrawVertices(D3DPRIMITIVETYPE type, const Transform* transform,
                    const D3DVECTOR* points, const TexCoord* coords,
                    D3DCOLOR color, int count);  // 0x409ed0

  // Transforms `count` points by `transform` and draws them untextured in
  // `color`.
  void DrawPoints(D3DPRIMITIVETYPE type, const Transform* transform,
                  const D3DVECTOR* points, D3DCOLOR color,
                  int count);  // 0x415040
  // Transforms `count` points by `transform` and draws them with the
  // texels `texels` of the current texture in `color`.
  void DrawTexels(D3DPRIMITIVETYPE type, const Transform* transform,
                  const D3DVECTOR* points, const POINTS* texels, D3DCOLOR color,
                  int count);  // 0x41c8e0
  // Point filtering for `stage` (SetFilter(FALSE, stage) as callers inline
  // it).
  void SetPointFilter(DWORD stage) {
    device_->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTFN_POINT);
    device_->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTFG_POINT);
  }
  // Draws without a texture from now on.
  void ClearTexture() {
    if (texture_ != NULL) {
      device_->SetTexture(0, NULL);
      texture_ = NULL;
    }
  }
  // Blends by the source alpha into `dest` times the screen.
  __forceinline void SetAlphaBlend(DWORD dest) {
    device_->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device_->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    device_->SetRenderState(D3DRENDERSTATE_DESTBLEND, dest);
  }

  void SetCullMode(D3DCULL mode) {
    device_->SetRenderState(D3DRENDERSTATE_CULLMODE, mode);
  }
  // Sets how texture coordinates outside 0..1 of `stage` are resolved.
  __forceinline void SetTextureAddress(DWORD mode, DWORD stage) {
    device_->SetTextureStageState(stage, D3DTSS_ADDRESSU, mode);
    device_->SetTextureStageState(stage, D3DTSS_ADDRESSV, mode);
  }

  // Draws `count` transformed vertices.
  void DrawPrimitive(D3DPRIMITIVETYPE type, D3DTLVERTEX* vertices, int count) {
    device_->DrawPrimitive(type, D3DFVF_TLVERTEX, vertices, count, 0);
  }

  void EndScene() {
    if (flags_ & kInScene) {
      device_->EndScene();
      flags_ &= ~kInScene;
    }
  }

 private:
  friend class ActorSystem;
  friend class HLMesh;
  friend void SetTexture(HL3D* d3d, HLSurface* texture, int stage);
  friend class HLCamera;
  friend class HLSTxtSys;
  friend class GameHud;

  // Creates the device `guid` (and its z-buffer) on the back buffer.
  // Returns 0, with the reason in g_text_buffer, when it cannot.
  int CreateDevice(REFCLSID guid, BOOL hardware);  // 0x402ed0
  // Allocates the vertices, the math tables and the mesh slots.
  int CreateBuffers();    // 0x402bf0
  void ReleaseBuffers();  // 0x4028d0
  // EnumZBufferFormats() callback: keeps in the DDSURFACEDESC2 `context`
  // the format closest to the depth asked for in its dwFlags (the distance
  // goes in dwWidth).
  static HRESULT CALLBACK EnumZBufferProc(DDPIXELFORMAT* format,
                                          void* context);  // 0x4029c0

  IDirect3D7* d3d_;               // 0x08
  IDirect3DDevice7* device_;      // 0x0c
  HLCamera* camera_;              // 0x10, in Application::overlays
  IDirectDrawSurface7* zbuffer_;  // 0x14
  short depth_;                   // 0x18, z-buffer bits
  Application* app_;              // 0x1c
  HLSurface* texture_;            // 0x20, set by SetTexture()
  unsigned int flags_;            // 0x24, Flags
  D3DMATERIAL7 material_;         // 0x28
  int vertex_count_;              // 0x6c, vertices used in vertices_
  D3DTLVERTEX* vertices_;         // 0x70, kMaxVertices
  HLMesh** meshes_;               // 0x74
};

// HL3D::SetTexture() (0x407880) as some callers inline it.
inline void SetTexture(HL3D* d3d, HLSurface* texture, int stage) {
  if (d3d->texture_ != texture) {
    if (texture == NULL) {
      d3d->device_->SetTexture(stage, NULL);
    } else {
      d3d->device_->SetTexture(stage, texture->surface());
    }
    d3d->texture_ = texture;
  }
}

// 0x41c8e0 (out of line where first used: fn_game.cpp)
inline void HL3D::DrawTexels(D3DPRIMITIVETYPE type, const Transform* transform,
                             const D3DVECTOR* points, const POINTS* texels,
                             D3DCOLOR color, int count) {
  if (texture_ == NULL) {
    return;
  }
  if (vertex_count_ + count > kMaxVertices) {
    vertex_count_ = 0;
  }
  float scale_u = 1.0f / (texture_->scale_x_ * texture_->width_);
  float scale_v = 1.0f / (texture_->scale_y_ * texture_->height_);
  D3DTLVERTEX* vertex = &vertices_[vertex_count_];
  for (int i = 0; i < count; ++i) {
    g_vector_work->point.x = transform->m[0][0] * points->x +
                             transform->m[1][0] * points->y +
                             transform->m[2][0];
    g_vector_work->point.y = transform->m[0][1] * points->x +
                             transform->m[1][1] * points->y +
                             transform->m[2][1];
    vertex->sx = g_vector_work->point.x;
    vertex->sy = g_vector_work->point.y;
    vertex->sz = points->z;
    vertex->tu = (texels->x + 0.45f) * scale_u;
    vertex->tv = (texels->y + 0.45f) * scale_v;
    vertex->color = color;
    ++points;
    ++texels;
    ++vertex;
  }
  device_->DrawPrimitive(type, D3DFVF_TLVERTEX, &vertices_[vertex_count_],
                         count, 0);
  vertex_count_ += count;
}

// 0x415040 (out of line where first used: line_sprites.cpp)
inline void HL3D::DrawPoints(D3DPRIMITIVETYPE type, const Transform* transform,
                             const D3DVECTOR* points, D3DCOLOR color,
                             int count) {
  if (vertex_count_ + count > kMaxVertices) {
    vertex_count_ = 0;
  }
  D3DTLVERTEX* vertex = &vertices_[vertex_count_];
  for (int i = 0; i < count; ++i) {
    g_vector_work->point.x = transform->m[0][0] * points->x +
                             transform->m[1][0] * points->y +
                             transform->m[2][0];
    g_vector_work->point.y = transform->m[0][1] * points->x +
                             transform->m[1][1] * points->y +
                             transform->m[2][1];
    vertex->sx = g_vector_work->point.x;
    vertex->sy = g_vector_work->point.y;
    vertex->sz = points->z;
    vertex->color = color;
    ++points;
    ++vertex;
  }
  device_->DrawPrimitive(type, D3DFVF_TLVERTEX, &vertices_[vertex_count_],
                         count, 0);
  vertex_count_ += count;
}

#endif  // WF_HL_3D_H_
