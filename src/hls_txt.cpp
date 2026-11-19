#include "hls_txt.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "application.h"
#include "hl_surface.h"

D3DCOLOR HLSTxtSys::default_palette_[10] = {
    0xff000000, 0xff0000ff, 0xffff0000, 0xffff00ff, 0xff00ff00,
    0xff00ffff, 0xffff00ff, 0xffffffff, 0xff555555, 0xffaaaaaa,
};

// 0x40acf0
HLSTxtSys::HLSTxtSys() {
  app_ = NULL;
  txts_ = NULL;
  texture_ = NULL;
  widths_ = NULL;
  name_ = "HLSTxtSys";
  palette_ = default_palette_;
}

// 0x40a9d0
void HLSTxt::Release() {
  if (flags_ & kOwnsText) {
    free(text_);
  }
  text_ = NULL;
  flags_ = 0;
}

// 0x40a9f0
int HLSTxtSys::Map(const char* character, int index) {
  if (mapping_count_ == kMaxMappings ||
      static_cast<unsigned short>(index) > char_count_) {
    return 0;
  }
  if (first_char_ <= 0xff) {
    mappings_[mapping_count_].code = static_cast<unsigned char>(character[0]);
  } else {
    mappings_[mapping_count_].code =
        (static_cast<unsigned char>(character[0]) << 8) +
        static_cast<unsigned char>(character[1]);
  }
  mappings_[mapping_count_].index = first_char_ + index;
  ++mapping_count_;
  return 1;
}

// 0x40aa60
void HLSTxtSys::ReleaseTexts() {
  if (txts_ == NULL) {
    return;
  }
  for (int i = 0; i < txt_count_; ++i) {
    txts_[i].Release();
  }
}

// 0x40aac0
HLSTxt* HLSTxtSys::Create(int size, char* text) {
  HLSTxt* txt = txts_;
  for (int i = txt_count_; i > 0; --i, ++txt) {
    if (txt->text_ == NULL) {
      if (text != NULL) {
        txt->text_ = text;
        return txt;
      }
      txt->text_ = static_cast<char*>(malloc(size));
      if (txt->text_ != NULL) {
        txt->flags_ |= HLSTxt::kOwnsText;
        txt->text_[0] = '\0';
        return txt;
      }
    }
  }
  return NULL;
}

// 0x40ab20
// TODO: nonmatching (close): `text` and the character swap esi/edi and
// eax/edx. Declaring `line` first keeps the height in memory as in the
// original.
void HLSTxtSys::Measure(SIZE* size, const char* text) {
  float line = 0.0f;
  float width = 0.0f;
  float height = char_height_;
  const Mapping* last = &mappings_[mapping_count_ - 1];
  for (;; ++text) {
    switch (*text) {
      case '\0':
        if (line > width) {
          width = line;
        }
        if (width == 0.0f) {
          size->cx = size->cy = 0;
        } else {
          size->cx = static_cast<LONG>(width);
          size->cy = static_cast<LONG>(height);
        }
        return;
      case '\n':
        height += char_height_;
        if (line > width) {
          width = line;
        }
        line = 0.0f;
        break;
      case '@':
        if (text[1] == '@') {
          text += 2;
          break;
        }
        // Fall through.
      default:
        if (widths_ != NULL) {
          unsigned short code = static_cast<unsigned char>(*text);
          if (first_char_ > 0xff) {
            code = (code << 8) + static_cast<unsigned char>(text[1]);
            ++text;
          }
          for (const Mapping* mapping = mappings_; mapping <= last; ++mapping) {
            if (code == mapping->code) {
              code = mapping->index;
              break;
            }
          }
          line += widths_[code - first_char_];
        } else {
          line += char_width_;
        }
        break;
      case ' ':
        line += space_width_;
        break;
    }
  }
}

// 0x40ad20
// TODO: nonmatching: the cell corners live in an 8-byte local in the
// original (with the left edge in a register); stack slots differ.
int HLSTxtSys::Initialize(Application* app, HLSurface* texture, int txt_count,
                          const char* first, short char_count, int u, int v,
                          int width, int height, int columns, int space_width,
                          float* widths) {
  char_width_ = static_cast<float>(width);
  app_ = app;
  char_height_ = static_cast<float>(height);
  texture_ = texture;
  txt_count_ = txt_count;
  char_count_ = char_count;
  mapping_count_ = 0;
  space_width_ = static_cast<float>(space_width > 0 ? space_width : width);
  widths_ = widths;
  if (strlen(first) == 1) {
    first_char_ = static_cast<unsigned char>(first[0]);
  } else {
    first_char_ = (static_cast<unsigned char>(first[0]) << 8) +
                  static_cast<unsigned char>(first[1]);
  }
  txts_ = new HLSTxt[txt_count_];
  if (txts_ == NULL) {
    goto failed;
  }
  for (int i = 0; i < txt_count_; ++i) {
    txts_[i].system_ = this;
    txts_[i].texture_ = texture_;
  }
  glyphs_ = static_cast<Glyph*>(malloc(char_count_ * sizeof(Glyph)));
  if (glyphs_ == NULL) {
  failed:
    // "Out of memory."
    Error("ﾒﾓﾘが足りません.");
    return 0;
  }
  {
    SMALL_RECT cell;
    cell.Left = u;
    cell.Top = v;
    cell.Right = u + width;
    cell.Bottom = v + height;
    int column = 0;
    for (int j = 0; j < char_count_; ++j) {
      GetTexCoord(texture_, &glyphs_[j].top_left, cell.Left, cell.Top);
      GetTexCoord(texture_, &glyphs_[j].bottom_right, cell.Right, cell.Bottom);
      if (++column >= columns) {
        cell.Top += height;
        cell.Bottom += height;
        cell.Left = u;
        cell.Right = u + width;
        column = 0;
      } else {
        cell.Left += width;
        cell.Right += width;
      }
    }
  }
  // "%d kinds of %d-byte characters (%dx%d) can be shown. Buffer=%d"
  Log("%d種の%dﾊﾞｲﾄ文字(%dx%d)を表示可能. Buffer=%d", char_count_,
      first_char_ > 0xff ? 2 : 1, width, height, txt_count_);
  return 1;
}

// 0x40b070
void HLSTxtSys::Release() {
  if (txts_ != NULL) {
    ReleaseTexts();
    delete[] txts_;
    txts_ = NULL;
  }
  if (glyphs_ != NULL) {
    free(glyphs_);
    glyphs_ = NULL;
  }
}

// 0x40b0d0
void HLSTxtSys::DrawGlyph(float x, float y, const Glyph* glyph,
                          D3DCOLOR color) {
  HL3D* d3d = app_->d3d;
  d3d->vertex_count_ += 4;
  D3DTLVERTEX* vertex = &d3d->vertices_[4];
  vertex[0].sx = vertex[2].sx = x;
  vertex[1].sx = vertex[3].sx = x + char_width_;
  vertex[0].sy = vertex[1].sy = y;
  vertex[2].sy = vertex[3].sy = y + char_height_;
  vertex[0].sz = vertex[1].sz = vertex[2].sz = vertex[3].sz = 0.5f;
  vertex[0].color = vertex[1].color = vertex[2].color = vertex[3].color = color;
  vertex[0].tu = vertex[2].tu = glyph->top_left.u;
  vertex[1].tu = vertex[3].tu = glyph->bottom_right.u;
  vertex[0].tv = vertex[1].tv = glyph->top_left.v;
  vertex[2].tv = vertex[3].tv = glyph->bottom_right.v;
  app_->d3d->DrawPrimitive(D3DPT_TRIANGLESTRIP, vertex, 4);
}

// 0x40b180
// TODO: nonmatching: the original spills the character code to a stack
// slot before the mapping search (an inline helper taking it by address?).
void HLSTxtSys::Print(float x, float y, const char* text,
                      const D3DCOLOR* palette, D3DCOLOR color, int count) {
  if (count == 0) {
    return;
  }
  if (count < 0) {
    count = 0x7fffffff;
  }
  if (palette == NULL) {
    palette = palette_;
  }
  unsigned short code;
  D3DCOLOR current = color;
  float left = x;
  const Mapping* last = &mappings_[mapping_count_ - 1];
  if (first_char_ <= 0xff) {
    for (;; ++text) {
      switch (*text) {
        case '\0':
          return;
        case '\n':
          y += char_height_;
          left = x;
          break;
        case '@':
          if (text[1] == '@') {
            if (isdigit(text[2])) {
              current = palette[text[2] - '0'];
            } else {
              current = color;
            }
            text += 2;
            break;
          }
          // Fall through.
        default: {
          code = static_cast<unsigned char>(*text);
          for (const Mapping* mapping = mappings_; mapping <= last; ++mapping) {
            if (code == mapping->code) {
              code = mapping->index;
              break;
            }
          }
          unsigned short index = code - first_char_;
          DrawGlyph(left, y, &glyphs_[index], current);
          if (--count <= 0) {
            return;
          }
          left += widths_ != NULL ? widths_[index] : char_width_;
          break;
        }
        case ' ':
          if (--count <= 0) {
            return;
          }
          left += space_width_;
          break;
      }
    }
  } else {
    for (;; ++text) {
      switch (*text) {
        case '\0':
          return;
        case '\n':
          y += char_height_;
          left = x;
          break;
        case '@':
          if (text[1] == '@') {
            if (isdigit(text[2])) {
              current = palette[text[2] - '0'];
            } else {
              current = color;
            }
            text += 2;
            break;
          }
          // Fall through.
        default: {
          code = (static_cast<unsigned char>(text[0]) << 8) +
                 static_cast<unsigned char>(text[1]);
          for (const Mapping* mapping = mappings_; mapping <= last; ++mapping) {
            if (code == mapping->code) {
              code = mapping->index;
              break;
            }
          }
          unsigned short index = code - first_char_;
          DrawGlyph(left, y, &glyphs_[index], current);
          if (--count <= 0) {
            return;
          }
          ++text;
          left += widths_ != NULL ? widths_[index] : char_width_;
          break;
        }
        case ' ':
          if (--count <= 0) {
            return;
          }
          left += space_width_;
          break;
      }
    }
  }
}

// 0x40b4d0
void HLSTxt::Draw(const Transform* transform, D3DCOLOR color, int blend) {
  system_->app_->d3d->SetTexture(texture_, 0);
  switch (blend) {
    case 1:
      system_->app_->d3d->SetBlend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_ONE);
      system_->Print(transform->m[2][0], transform->m[2][1], text_, 0, color,
                     -1);
      system_->app_->d3d->SetBlend(TRUE, D3DBLEND_SRCALPHA,
                                   D3DBLEND_INVSRCALPHA);
      break;
    default:
      system_->Print(transform->m[2][0], transform->m[2][1], text_, 0, color,
                     -1);
      break;
  }
}
