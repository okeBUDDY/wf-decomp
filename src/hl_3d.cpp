#include "hl_3d.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "graphics.h"
#include "hl_camera.h"
#include "hl_mesh.h"
#include "hl_surface.h"
#include "profile.h"
#include "task.h"

// 0x402bb0
HL3D::HL3D() {
  app_ = NULL;
  d3d_ = NULL;
  device_ = NULL;
  camera_ = NULL;
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
    Error("ﾒﾓﾘが足りません");
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
    Error("ﾒﾓﾘが足りません");
    return 0;
  }
  g_base_matrix.m[0][0] = g_base_matrix.m[1][1] = g_base_matrix.m[2][2] = 1.0f;
  g_base_matrix.m[0][1] = g_base_matrix.m[0][2] = g_base_matrix.m[1][0] =
      g_base_matrix.m[1][2] = g_base_matrix.m[2][0] = g_base_matrix.m[2][1] =
          0.0f;
  g_sin_table = static_cast<float*>(malloc(0x500 * sizeof(float)));
  g_angle_table = static_cast<float*>(malloc(0x400 * sizeof(float)));
  if (g_sin_table == NULL || g_angle_table == NULL) {
    Error("ﾒﾓﾘが足りません");
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
    Error("メモリが足りません");
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
      strcpy(g_text_buffer, "Zﾊﾞｯﾌｧが利用できません");
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
      strcpy(g_text_buffer, "Zﾊﾞｯﾌｧが生成できません");
      goto failed;
    }
    if (app_->screen->surface_->AddAttachedSurface(zbuffer_) != DD_OK) {
      // "Cannot attach the z-buffer to the surface"
      strcpy(g_text_buffer, "Zﾊﾞｯﾌｧをｻｰﾌｪｽにｱﾀｯﾁできません");
      goto failed;
    }
  }
  if (d3d_->CreateDevice(guid, app_->screen->surface_, &device_) != D3D_OK) {
    // "Direct3DDevice is not available"
    strcpy(g_text_buffer, "Direct3DDeviceが利用できません");
    goto failed;
  }
  if (!app_->screen->InitTextureFormats(device_)) {
    // "A required texture format does not exist"
    strcpy(g_text_buffer, "必要なﾃｸｽﾁｬ形式が存在しません");
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

// 0x4030c0
int HL3D::Initialize(Application* app) {
  app_ = app;
  if (app->graphics == NULL || app->graphics->ddraw_ == NULL ||
      app->screen == NULL) {
    Error("HLDrawが初期化されていません");
    return 0;
  }
  if (app_->graphics->ddraw_->QueryInterface(
          IID_IDirect3D7, reinterpret_cast<void**>(&d3d_)) != DD_OK) {
    Error("Direct3D7が利用できません");
    return 0;
  }
  flags_ = 0;
  if (CreateDevice(IID_IDirect3DTnLHalDevice, TRUE)) {
    Log("Direct3D-HAL(TnL)ﾃﾞﾊﾞｲｽを使用 : Zﾊﾞｯﾌｧ=%dbit", depth_);
    flags_ |= kTnLDevice;
  } else if (CreateDevice(IID_IDirect3DHALDevice, TRUE)) {
    Log("Direct3D-HALﾃﾞﾊﾞｲｽを使用 : Zﾊﾞｯﾌｧ=%dbit", depth_);
  } else {
    app_->flags |= AppSettings::kNoHardware3D;
    if (CreateDevice(IID_IDirect3DMMXDevice, FALSE)) {
      Log("Direct3D-HEL(MMX)ﾃﾞﾊﾞｲｽを使用 : Zﾊﾞｯﾌｧ=%dbit", depth_);
    } else if (CreateDevice(IID_IDirect3DRGBDevice, FALSE)) {
      Log("Direct3D-HEL(RGB)ﾃﾞﾊﾞｲｽを使用 : Zﾊﾞｯﾌｧ=%dbit", depth_);
    } else {
      Error(g_text_buffer);
      return 0;
    }
  }
  flags_ |= HLSurface::texture_caps_ & 0xffff;

  D3DVIEWPORT7 viewport;
  viewport.dwX = 0;
  viewport.dwY = 0;
  viewport.dwWidth = app_->width;
  viewport.dwHeight = app_->height;
  viewport.dvMinZ = 0;
  viewport.dvMaxZ = 1;
  if (device_->SetViewport(&viewport) != D3D_OK) {
    Error("ﾋﾞｭｰﾎﾟｰﾄが設定できません");
    return 0;
  }
  camera_ = new HLCamera;
  if (camera_ == NULL) {
    Error("ｶﾒﾗｼｽﾃﾑが初期化できません");
    return 0;
  }
  HLCamera::d3d_ = this;
  app_->overlays.Add(camera_, 0, TASK_APPEND);
  TaskBase* node = new TaskBase;
  Node::d3d_ = this;
  delete node;
  if (!CreateBuffers()) {
    return 0;
  }

  device_->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_NONE);
  device_->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
  device_->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
  device_->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
  device_->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
  device_->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
  device_->SetRenderState(D3DRENDERSTATE_COLORVERTEX, FALSE);
  device_->SetRenderState(D3DRENDERSTATE_LOCALVIEWER, FALSE);
  if (zbuffer_ != NULL) {
    device_->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    device_->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    device_->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
  }
  device_->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xff000000);
  device_->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
  SetLighting(TRUE);
  ZeroMemory(&material_, sizeof(material_));
  material_.diffuse.r = material_.diffuse.g = material_.diffuse.b = 1.0f;
  material_.ambient.r = material_.ambient.g = material_.ambient.b = 1.0f;
  device_->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
  device_->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
  device_->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0);
  device_->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
  device_->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
  device_->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  device_->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
  device_->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
  SetAlpha(1.0f);
  device_->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
  SetTextureOp(D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, 0);
  device_->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
  device_->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
  Clear();
  return 1;
}
