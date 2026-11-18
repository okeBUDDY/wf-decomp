// The fonts the game loads at startup (cGlobal::LoadFonts() 0x401280).
#ifndef WF_FONTS_H_
#define WF_FONTS_H_

#include "hl_surface.h"
#include "hls_txt.h"

extern HLSurface g_font0_texture;  // 0x467ec0, "font0.png"
extern HLSurface g_font1_texture;  // 0x467ee8, "font1.png"
extern HLSurface g_font2_texture;  // 0x467f10, "font2.png"

extern HLSTxtSys g_text_font;         // 0x467f38, 16x16 ASCII
extern HLSTxtSys g_small_font;        // 0x467fac, 8x8 ASCII
extern HLSTxtSys g_kana_font;         // 0x468020, 16x32 katakana
extern HLSTxtSys g_large_digit_font;  // 0x468108, 48x80 digits
extern HLSTxtSys g_digit_font;        // 0x46817c, 24x40 digits
// g_title_font (hls_txt.h): 20x32 capitals.
extern float g_title_font_widths[28];  // 0x45d100, of each capital and '-'

#endif  // WF_FONTS_H_
