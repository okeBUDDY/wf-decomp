#include "hls_pat.h"

#include <stdlib.h>

#include "application.h"
#include "hls_hit_image.h"

// 0x409bb0
void HL3D::GetTexCoord(HLSurface* texture, TexCoord* out, const POINTS* point) {
  out->u = (point->x + 0.45f) / (texture->scale_x_ * texture->width_);
  out->v = (point->y + 0.45f) / (texture->scale_y_ * texture->height_);
}

// 0x409c40
void HLSPat::Hit(HLSHitImage* image, int x, int y, DWORD id) {
  image->Fill(x, y, hit_width_, hit_height_, id);
}

// 0x409c70
int HLSPat::Setup(int width, int height, HLSurface* texture, short u, short v,
                  int x, int y, int hit_width, int hit_height) {
  if (texture == NULL) {
    texture_ = new HLSurface;
    if (texture_ == NULL ||
        !texture_->Create(width, height, HLSurface::kTexture | 0x40)) {
      return 0;
    }
    owns_texture_ = true;
  } else {
    texture_ = texture;
  }
  used_ = 1;
  hit_width_ = hit_width >> 1;
  hit_height_ = hit_height >> 1;
  if ((width | height) == 0) {
    empty_ = true;
    return 1;
  }
  points_[0].x = points_[2].x = static_cast<float>(-x);
  points_[0].y = points_[1].y = static_cast<float>(-y);
  points_[1].x = points_[3].x = static_cast<float>(width - x);
  points_[2].y = points_[3].y = static_cast<float>(height - y);
  POINTS texel;
  texel.x = u;
  texel.y = v;
  d3d_->GetTexCoord(texture_, &coords_[0], &texel);
  texel.x = u + width;
  texel.y = v + height;
  d3d_->GetTexCoord(texture_, &coords_[3], &texel);
  coords_[1].u = coords_[3].u;
  coords_[1].v = coords_[0].v;
  coords_[2].u = coords_[0].u;
  coords_[2].v = coords_[3].v;
  return 1;
}

// 0x409e20
void HLSPat::Draw(const Transform* transform, D3DCOLOR color, int blend) {
  if (empty_) {
    return;
  }
  if (!(color & 0xff000000)) {
    return;
  }
  d3d_->SetTexture(texture_, 0);
  switch (blend) {
    case 1:
      d3d_->SetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_ONE);
      d3d_->DrawVertices(D3DPT_TRIANGLESTRIP, transform, points_, coords_,
                         color, 4);
      d3d_->SetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
      break;
    default:
      d3d_->DrawVertices(D3DPT_TRIANGLESTRIP, transform, points_, coords_,
                         color, 4);
      break;
  }
}

// 0x409ed0
void HL3D::DrawVertices(D3DPRIMITIVETYPE type, const Transform* transform,
                        const D3DVECTOR* points, const TexCoord* coords,
                        D3DCOLOR color, int count) {
  if (vertex_count_ + count > kMaxVertices) {
    vertex_count_ = 0;
  }
  D3DTLVERTEX* vertex = &vertices_[vertex_count_];
  for (int i = count; i > 0; --i) {
    TransformPoint(vertex, transform, points);
    vertex->tu = coords->u;
    vertex->tv = coords->v;
    vertex->color = color;
    ++points;
    ++coords;
    ++vertex;
  }
  device_->DrawPrimitive(type, D3DFVF_TLVERTEX, &vertices_[vertex_count_],
                         count, 0);
  vertex_count_ += count;
}

// 0x409f80
HLSPat* HLSPatSys::Create(int width, int height, HLSurface* texture, short u,
                          short v, int x, int y, int hit_width,
                          int hit_height) {
  HLSPat* pat = pats_;
  for (int i = count_; i > 0; --i) {
    if (pat->used() == 0) {
      if (pat->Setup(width, height, texture, u, v, x, y, hit_width,
                     hit_height)) {
        return pat;
      }
    }
    ++pat;
  }
  return NULL;
}

// 0x40a000
HLSPatFile::HLSPatFile() {
  name_ = "  HLSPatFile";
  count_ = 0;
  pats_ = NULL;
}

// 0x40a060
void HLSPatFile::Release() {
  if (pats_ != NULL) {
    for (int i = 0; i < count_; ++i) {
      if (pats_[i] != NULL) {
        pats_[i]->Release();
        pats_[i] = NULL;
      }
    }
    if (pats_ != NULL) {
      free(pats_);
    }
    pats_ = NULL;
    count_ = 0;
  }
  texture_.Release();
}

// 0x40a110
void HLSPatSys::Finalize() {
  if (pats_ == NULL) {
    return;
  }
  for (int i = 0; i < count_; ++i) {
    HLSPat* pat = &pats_[i];
    if (pat != NULL) {
      pat->Release();
    }
  }
  delete[] pats_;
  pats_ = NULL;
}

// 0x40a890
int HLSPatSys::Initialize(Application* app) {
  app_ = app;
  pats_ = new HLSPat[count_];
  if (pats_ == NULL) {
    // "Out of memory"
    Error("\322\323\330\202\252\221\253\202\350\202\334\202\271\202\361");
    return 0;
  }
  HLSPat::d3d_ = app_->d3d;
  HLSPat::system_ = this;
  // "%d patterns available."
  Log("%d\312\337\300\260\335\227\230\227p\211\302\224\\.", count_);
  return 1;
}
