#include "hl_3d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "graphics.h"
#include "hl_mesh.h"
#include "hl_surface.h"
#include "profile.h"

// 0x402bb0
HL3D::HL3D() {
  app_ = NULL;
  d3d_ = NULL;
  device_ = NULL;
  unknown_10_ = 0;
  zbuffer_ = NULL;
  texture_ = NULL;
  flags_ = 0;
  vertices_ = NULL;
  vertex_count_ = 0;
  name_ = "HL3D";
  depth_ = 16;
}

// 0x4028d0
void HL3D::ReleaseBuffers() {
  for (int i = 0; i < kMaxMeshes; ++i) {
    if (meshes_[i] != NULL) {
      meshes_[i]->Release();
    }
  }
  if (meshes_ != NULL) {
    free(meshes_);
  }
  meshes_ = NULL;
  if (g_sin_table != NULL) {
    free(g_sin_table);
  }
  g_sin_table = NULL;
  if (g_angle_table != NULL) {
    free(g_angle_table);
  }
  g_angle_table = NULL;
  if (g_vector_work != NULL) {
    free(g_vector_work);
  }
  g_vector_work = NULL;
  if (g_matrix_work != NULL) {
    free(g_matrix_work);
  }
  g_matrix_work = NULL;
  if (vertices_ != NULL) {
    free(vertices_);
  }
  vertices_ = NULL;
}

// 0x402990
void HL3D::Resume() {
  if (zbuffer_ != NULL && zbuffer_->IsLost() == DDERR_SURFACELOST) {
    zbuffer_->Restore();
  }
}

// 0x4029c0
HRESULT CALLBACK HL3D::EnumZBufferProc(DDPIXELFORMAT* format, void* context) {
  DDSURFACEDESC2* desc = static_cast<DDSURFACEDESC2*>(context);
  DWORD distance =
      abs(static_cast<int>(format->dwZBufferBitDepth - desc->dwFlags));
  if (distance < desc->dwWidth) {
    desc->ddpfPixelFormat = *format;
    desc->dwWidth = distance;
    if (distance == 0) {
      return D3DENUMRET_CANCEL;
    }
  }
  return D3DENUMRET_OK;
}

// 0x402a00
void HL3D::SetAlpha(float alpha) {
  material_.diffuse.a = alpha;
  device_->SetMaterial(&material_);
}

// 0x402a50
void HL3D::SetBlend(BOOL enable, DWORD src, DWORD dest) {
  device_->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, enable);
  if (enable) {
    device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  } else {
    device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  }
  if (src != 0) {
    device_->SetRenderState(D3DRENDERSTATE_SRCBLEND, src);
  }
  if (dest != 0) {
    device_->SetRenderState(D3DRENDERSTATE_DESTBLEND, dest);
  }
}

// 0x402ac0
void HL3D::SetTextureOp(DWORD op, DWORD arg1, DWORD arg2, DWORD stage) {
  device_->SetTextureStageState(stage, D3DTSS_COLOROP, op);
  device_->SetTextureStageState(stage, D3DTSS_ALPHAOP, op);
  if (arg1 != 0) {
    device_->SetTextureStageState(stage, D3DTSS_COLORARG1, arg1);
    device_->SetTextureStageState(stage, D3DTSS_ALPHAARG1, arg1);
  }
  if (arg2 != 0) {
    device_->SetTextureStageState(stage, D3DTSS_COLORARG2, arg2);
    device_->SetTextureStageState(stage, D3DTSS_ALPHAARG2, arg2);
  }
}

// 0x402b50
void HL3D::SetFilter(BOOL linear, DWORD stage) {
  if (linear) {
    device_->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    device_->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
  } else {
    device_->SetTextureStageState(stage, D3DTSS_MINFILTER, D3DTFN_POINT);
    device_->SetTextureStageState(stage, D3DTSS_MAGFILTER, D3DTFG_POINT);
  }
}

// 0x402bf0
int HL3D::CreateBuffers() {
  vertices_ =
      static_cast<D3DTLVERTEX*>(malloc(kMaxVertices * sizeof(D3DTLVERTEX)));
  if (vertices_ == NULL) {
    // "Out of memory"
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  vertex_count_ = 0;
  for (int i = 0; i < kMaxVertices; ++i) {
    vertices_[i].rhw = 1.0f;
    vertices_[i].specular = 0xffffffff;
  }
  g_vector_work = static_cast<float*>(malloc(0x78));
  g_matrix_work = static_cast<float*>(malloc(0x280));
  if (g_vector_work == NULL || g_matrix_work == NULL) {
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  g_base_matrix[0][0] = g_base_matrix[1][1] = g_base_matrix[2][2] = 1.0f;
  g_base_matrix[0][1] = g_base_matrix[0][2] = g_base_matrix[1][0] =
      g_base_matrix[1][2] = g_base_matrix[2][0] = g_base_matrix[2][1] = 0.0f;
  g_sin_table = static_cast<float*>(malloc(0x500 * sizeof(float)));
  g_angle_table = static_cast<float*>(malloc(0x400 * sizeof(float)));
  if (g_sin_table == NULL || g_angle_table == NULL) {
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  for (int i = 0; i < 0x500; ++i) {
    if (i < 0x400) {
      g_angle_table[i] = i * (2.0f * 3.14159265f / 1024.0f);
    }
    g_sin_table[i] = sin(i * (2.0f * 3.14159265f / 1024.0f));
  }
  HLMesh mesh;
  HLMesh::d3d_ = this;
  meshes_ = static_cast<HLMesh**>(malloc(kMaxMeshes * sizeof(HLMesh*)));
  if (meshes_ == NULL) {
    // "Out of memory"
    Error(
        "\203\201\203\202\203\212\202\252\221\253\202\350\202\334\202\271\202"
        "\361");
    return 0;
  }
  for (int j = 0; j < kMaxMeshes; ++j) {
    meshes_[j] = NULL;
  }
  return 1;
}

// 0x402e60
void HL3D::Finalize() {
  ReleaseBuffers();
  for (DWORD stage = 0; stage < 8; ++stage) {
    device_->SetTexture(stage, NULL);
  }
  if (device_ != NULL) {
    device_->Release();
  }
  device_ = NULL;
  if (zbuffer_ != NULL) {
    app_->screen->surface_->DeleteAttachedSurface(0, zbuffer_);
    zbuffer_->Release();
  }
  zbuffer_ = NULL;
  if (d3d_ != NULL) {
    d3d_->Release();
  }
  d3d_ = NULL;
}

// 0x402ed0
int HL3D::CreateDevice(REFCLSID guid, BOOL hardware) {
  if (hardware && (app_->flags & AppSettings::kNoHardware3D)) {
    return 0;
  }
  DDSURFACEDESC2 desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.dwSize = sizeof(desc);
  if (depth_ != 0) {
    // EnumZBufferProc() takes the depth in dwFlags, the distance in dwWidth.
    desc.dwFlags = depth_;
    desc.dwWidth = 0x10000;
    if (d3d_->EnumZBufferFormats(guid, EnumZBufferProc, &desc) != D3D_OK ||
        desc.dwWidth == 0x10000) {
      // "No z-buffer is available"
      strcpy(g_text_buffer,
             "Z\312\336\257\314\247\202\252\227\230\227p\202\305\202\253"
             "\202\334\202\271\202\361");
      goto failed;
    }
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.ddsCaps.dwCaps =
        (hardware ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY) |
        DDSCAPS_ZBUFFER;
    desc.dwWidth = app_->width;
    desc.dwHeight = app_->height;
    if (app_->graphics->ddraw_->CreateSurface(&desc, &zbuffer_, NULL) !=
        DD_OK) {
      // "Cannot create the z-buffer"
      strcpy(g_text_buffer,
             "Z\312\336\257\314\247\202\252\220\266\220\254\202\305\202"
             "\253\202\334\202\271\202\361");
      goto failed;
    }
    if (app_->screen->surface_->AddAttachedSurface(zbuffer_) != DD_OK) {
      // "Cannot attach the z-buffer to the surface"
      strcpy(g_text_buffer,
             "Z\312\336\257\314\247\202\360\273\260\314\252\275\202\311"
             "\261\300\257\301\202\305\202\253\202\334\202\271\202\361");
      goto failed;
    }
  }
  if (d3d_->CreateDevice(guid, app_->screen->surface_, &device_) != D3D_OK) {
    // "Direct3DDevice is not available"
    strcpy(g_text_buffer,
           "Direct3DDevice\202\252\227\230\227p\202\305\202\253\202\334"
           "\202\271\202\361");
    goto failed;
  }
  if (!app_->screen->InitTextureFormats(device_)) {
    // "A required texture format does not exist"
    strcpy(g_text_buffer,
           "\225K\227v\202\310\303\270\275\301\254\214`\216\256\202\252"
           "\221\266\215\335\202\265\202\334\202\271\202\361");
    goto failed;
  }
  depth_ = static_cast<short>(desc.ddpfPixelFormat.dwZBufferBitDepth);
  return 1;

failed:
  if (zbuffer_ != NULL) {
    app_->screen->surface_->DeleteAttachedSurface(0, zbuffer_);
    zbuffer_->Release();
  }
  zbuffer_ = NULL;
  return 0;
}

// 0x4030a0
void HL3D::SetLighting(BOOL enable) {
  device_->SetRenderState(D3DRENDERSTATE_LIGHTING, enable);
}
