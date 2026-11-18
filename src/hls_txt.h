// Bitmap text: HLSTxtSys holds a font (a grid of characters on a texture)
// and the pool of HLSTxt objects that draw strings with it.
#ifndef WF_HLS_TXT_H_
#define WF_HLS_TXT_H_

#include <windows.h>

#include <d3d.h>

#include "hl_3d.h"
#include "hls_anm.h"

struct Application;
class HLSTxtSys;

// The texture rectangle of one character.
struct Glyph {
  TexCoord top_left;
  TexCoord bottom_right;
};

// vtable 0x451f9c.
class HLSTxt : public HLSObject {
 public:
  enum Flags { kOwnsText = 0x1 };

  HLSTxt() {
    text_ = NULL;
    system_ = NULL;
    flags_ = 0;
  }  // 0x40acd0
  // Draws the text at the origin of `transform`.
  virtual void Draw(const Transform* transform, D3DCOLOR color,
                    int blend);  // 0x40b4d0

  // Copies `text` into the text buffer.
  void SetText(const char* text);  // 0x41ca50
  // The size of the text in pixels, 0 x 0 without one.
  void Measure(SIZE* size);  // 0x420480
  // Print `count` characters (all when negative) at (`x`, `y`) in `color`
  // or in the colors of `palette` (its first one for plain text).
  void Print(int x, int y, D3DCOLOR color, int count);           // 0x41ca70
  void Print(int x, int y, const D3DCOLOR* palette, int count);  // 0x41dfd0
  // Frees the text object.
  void Release();  // 0x40a9d0

 private:
  friend class HLSTxtSys;

  // Makes the font texture current (HL3D::SetTexture(), inlined).
  inline void BindTexture();

  unsigned int flags_;  // 0x08, Flags
  char* text_;          // 0x0c, NULL while the object is free
  HLSTxtSys* system_;   // 0x10
};

class HLSTxtSys : public Object {
 public:
  enum { kMaxMappings = 16 };

  HLSTxtSys();  // 0x40acf0

  // Sets up the font: `char_count` characters from `first` (a one or two
  // byte character), `width` x `height` each, in rows of `columns` from
  // (`u`, `v`) of `texture`; a space is `space_width` wide (`width` when
  // not positive) and `widths` has the width of each character (all
  // `width` when NULL). Makes `txt_count` text objects.
  int Initialize(Application* app, HLSurface* texture, short txt_count,
                 const char* first, short char_count, int u, int v, int width,
                 int height, int columns, int space_width,
                 float* widths);  // 0x40ad20
  void Release();                 // 0x40b070
  // Draws `character` with the glyph `index` (at most kMaxMappings).
  // Returns 0 when it cannot.
  int Map(const char* character, int index);  // 0x40a9f0
  // A free text object, holding `text` or a new buffer of `size` bytes.
  HLSTxt* Create(int size, char* text);  // 0x40aac0
  // The size of `text` in pixels.
  void Measure(SIZE* size, const char* text);  // 0x40ab20
  // Draws at most `count` (all when negative) characters of `text` at
  // (`x`, `y`) in `color`. "@@<digit>" switches to that color of `palette`
  // (palette_ when NULL), "@@" followed by anything else back to `color`.
  void Print(float x, float y, const char* text, const D3DCOLOR* palette,
             D3DCOLOR color, int count);  // 0x40b180

  static D3DCOLOR default_palette_[10];  // 0x45d280

 private:
  friend class HLSTxt;

  struct Mapping {
    unsigned short code;
    unsigned short index;
  };

  void ReleaseTexts();  // 0x40aa60
  // Draws `glyph` at (`x`, `y`).
  void DrawGlyph(float x, float y, const Glyph* glyph,
                 D3DCOLOR color);  // 0x40b0d0

  Application* app_;                // 0x08
  HLSTxt* txts_;                    // 0x0c, new[]ed
  HLSurface* texture_;              // 0x10
  Glyph* glyphs_;                   // 0x14, malloc()ed
  float char_width_;                // 0x18
  float char_height_;               // 0x1c
  float space_width_;               // 0x20
  short txt_count_;                 // 0x24
  unsigned short first_char_;       // 0x26
  short char_count_;                // 0x28
  short mapping_count_;             // 0x2a
  Mapping mappings_[kMaxMappings];  // 0x2c
  const D3DCOLOR* palette_;         // 0x6c, for "@@<digit>"
  float* widths_;                   // 0x70
};

// The font the boss's title is measured in (one of five at 0x467fac,
// 0x74 bytes apart).
extern HLSTxtSys g_title_font;  // 0x468094

#endif  // WF_HLS_TXT_H_
