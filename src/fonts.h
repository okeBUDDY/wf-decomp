// The fonts the game loads at startup (cGlobal::LoadFonts() 0x401280).
#ifndef WF_FONTS_H_
#define WF_FONTS_H_

#include "hl_surface.h"
#include "hls_txt.h"

// One global object: its implicit ctor (0x401e90) and dtor (0x401f80) are
// out of line, which separate globals would not have.
struct Fonts {
  HLSurface textures[3];  // 0x000, "font0.png".."font2.png"
  HLSTxtSys text;         // 0x078, 16x16 ASCII
  HLSTxtSys small_text;   // 0x0ec, 8x8 ASCII ("small" is a macro)
  HLSTxtSys kana;         // 0x160, 16x32 katakana
  HLSTxtSys title;        // 0x1d4, 20x32 capitals (the boss's title)
  HLSTxtSys large_digit;  // 0x248, 48x80 digits
  HLSTxtSys digit;        // 0x2bc, 24x40 digits
};

extern Fonts g_fonts;                  // 0x467ec0
extern float g_title_font_widths[28];  // 0x45d100, of each capital and '-'

#endif  // WF_FONTS_H_
