// The title of the planned boss.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enm_param.h"
#include "hls_txt.h"

// 0x417c10
void CEnmParam::ChooseTitle() {
  EnmTitle title;
  memset(&title, 0xff, sizeof(title));
  if (state_.sides[0].mode == 0 && state_.sides[1].mode == 0) {
    title.words[0] = 0;
  } else if (state_.sides[0].mode == 3 || state_.sides[1].mode == 3) {
    title.words[0] = state_.title.words[0];
    if (state_.sides[0].mode == 4) {
      if (state_.sides[1].progress >= 100) {
        title.words[0] = 0x17;
      }
    } else if (state_.sides[1].mode == 4) {
      if (state_.sides[0].progress >= 100) {
        title.words[0] = 0x18;
      }
    } else if (state_.sides[0].progress >= 100 &&
               state_.sides[1].progress >= 100) {
      title.words[0] = 0x19;
    }
  } else if (state_.sides[0].progress >= 100 &&
             state_.sides[1].progress >= 100) {
    switch (state_.sides[0].mode) {
      case 1:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0xe;
            break;
          case 2:
            title.words[0] = 0xf;
            break;
          case 4:
            title.words[0] = 0x10;
            break;
        }
        break;
      case 2:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0x11;
            break;
          case 2:
            title.words[0] = 0x12;
            break;
          case 4:
            title.words[0] = 0x13;
            break;
        }
        break;
      case 4:
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 0x14;
            break;
          case 2:
            title.words[0] = 0x15;
            break;
          case 4:
            title.words[0] = 0x16;
            break;
        }
        break;
    }
  } else {
    if (state_.sides[0].progress >= 0x41) {
      switch (state_.sides[0].mode) {
        case 1:
          title.words[1] = 0xb;
          break;
        case 2:
          title.words[1] = 0xc;
          break;
        case 4:
          title.words[1] = 0xd;
          break;
      }
    } else if (state_.sides[0].mode != 0) {
      switch (state_.sides[0].mode) {
        case 1:
          title.words[1] = 4;
          break;
        case 2:
          title.words[1] = 5;
          break;
        case 4:
          title.words[1] = 6;
          break;
      }
    }
    if (state_.sides[1].progress >= 0x41) {
      switch (state_.sides[1].mode) {
        case 1:
          title.words[0] = 8;
          break;
        case 2:
          title.words[0] = 9;
          break;
        case 4:
          title.words[0] = 0xa;
          break;
      }
    } else if (state_.sides[1].mode != 0) {
      if (state_.sides[0].mode == 0) {
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 4;
            break;
          case 2:
            title.words[0] = 5;
            break;
          case 4:
            title.words[0] = 6;
            break;
        }
      } else if (state_.sides[1].mode == state_.sides[0].mode &&
                 state_.sides[0].progress < 0x41) {
        title.words[0] = 7;
      } else {
        switch (state_.sides[1].mode) {
          case 1:
            title.words[0] = 1;
            break;
          case 2:
            title.words[0] = 2;
            break;
          case 4:
            title.words[0] = 3;
            break;
        }
      }
    }
  }

  EnmTitle* seen = titles_;
  int count = title_count_;
  int i;
  for (i = 0; i < title_count_; ++i, ++seen) {
    if (seen->words[0] == title.words[0] && seen->words[1] == title.words[1]) {
      break;
    }
  }
  if (i != count) {
    if (new_weapon_ >= 0 && weapon_parts_[new_weapon_] != 0) {
      title.weapon = new_weapon_;
      title.level = weapon_parts_[new_weapon_] * 2;
    } else {
      int most = -1;
      int main;
      int kinds = 0;
      int j;
      for (j = 0; j < 7; ++j) {
        if (weapon_parts_[j] > 0) {
          if (weapon_parts_[j] > most) {
            most = weapon_parts_[j];
            main = j;
          }
          ++kinds;
        }
      }
      for (j = 0; j < 7; ++j) {
        if (j != main && most - weapon_parts_[j] < 3) {
          break;
        }
      }
      if (j == 7) {
        title.weapon = main;
        title.level = most * 2;
      } else {
        title.weapon = 7;
        title.level = kinds;
      }
    }
    if (title.level > 20) {
      title.level = 20;
    }
    seen = titles_;
    for (i = 0; i < title_count_; ++i, ++seen) {
      if (seen->words[0] == title.words[0] &&
          seen->words[1] == title.words[1] && seen->weapon == title.weapon &&
          seen->level == title.level) {
        break;
      }
    }
  }
  if (i == count) {
    if (count >= 32) {
      title.variant = 20;
    } else {
      titles_[count] = title;
      ++title_count_;
    }
  } else {
    if (seen->variant >= 0) {
      if (++seen->variant >= 20) {
        seen->variant = 0;
      }
    } else {
      seen->variant = rand() % 20;
    }
    title.variant = seen->variant;
  }
  state_.title = title;
}

// A word of a title (8 bytes).
struct EnmTitleWord {
  const char* text;
  int alone;  // the word makes the title on its own, before the weapon
};

// The words of the titles, in Japanese (katakana).
extern EnmTitleWord g_title_words_ja[];   // 0x45d5e0
extern const char* g_weapon_words_ja[];   // 0x45d6b0
extern const char* g_level_words_ja[];    // 0x45d6d0, by weapon level
extern const char* g_variant_words_ja[];  // 0x45d728

// "The" in front of `word`: "ジ" before a vowel (ァ to ェ), else "ザ".
static inline const char* TheJa(const char* word) {
  if (static_cast<unsigned char>(word[0]) == 0x83 &&
      static_cast<unsigned char>(word[1]) >= 0x40 &&
      static_cast<unsigned char>(word[1]) <= 0x49) {
    return "ジ";
  }
  return "ザ";
}

// 0x418030
void CEnmParam::FormatTitleJa(char* out) {
  char name[64] = "", weapon[64] = "";
  if (state_.title.words[0] >= 0) {
    strcat(name, g_title_words_ja[state_.title.words[0]].text);
  }
  if (state_.title.words[1] >= 0) {
    strcat(name, g_title_words_ja[state_.title.words[1]].text);
  }
  short level = state_.title.level;
  if (level >= 0) {
    strcat(weapon, g_level_words_ja[level]);
  }
  short kind = state_.title.weapon;
  if (kind >= 0) {
    strcat(weapon, g_weapon_words_ja[kind]);
  }
  int length;
  if (level < 0) {
    sprintf(out, "%s　%s", TheJa(name), name);
    length = strlen(name) + 4;
  } else if (g_title_words_ja[state_.title.words[0]].alone) {
    length = strlen(name) + strlen(weapon) + 6;
    if (length > 30) {
      sprintf(out, "%s　%s\n%s", name, TheJa(weapon), weapon);
      length = strlen(weapon);
    } else {
      sprintf(out, "%s　%s　%s", name, TheJa(weapon), weapon);
    }
  } else {
    const char* of = kind == 7 ? "ド" : "";
    length = strlen(weapon) + strlen(of) + strlen(name);
    if (length > 30) {
      sprintf(out, "%s%s\n%s", weapon, of, name);
      length = strlen(name);
    } else {
      sprintf(out, "%s%s%s", weapon, of, name);
    }
  }
  if (state_.title.variant >= 0) {
    const char* variant = g_variant_words_ja[state_.title.variant];
    if (strlen(variant) + length + 2 > 30) {
      strcat(out, "\n");
    } else {
      strcat(out, "　");
    }
    strcat(out, variant);
  }
}

// The words of the titles, in English.
extern EnmTitleWord g_title_words_en[];   // 0x45d780
extern const char* g_weapon_words_en[];   // 0x45d850
extern const char* g_level_words_en[];    // 0x45d870
extern const char* g_variant_words_en[];  // 0x45d8c8

// 0x418400
void CEnmParam::FormatTitleEn(char* out) {
  char name[64] = "", weapon[64] = "";
  char first = state_.title.words[0];
  if (first >= 0) {
    strcat(name, g_title_words_en[first].text);
  }
  if (state_.title.words[1] >= 0) {
    strcat(name, g_title_words_en[state_.title.words[1]].text);
  }
  short level = state_.title.level;
  if (level >= 0) {
    strcat(weapon, g_level_words_en[level]);
  }
  if (state_.title.weapon >= 0) {
    strcat(weapon, g_weapon_words_en[state_.title.weapon]);
  }
  SIZE size;
  if (level < 0) {
    sprintf(out, "THE %s", name);
  } else if (g_title_words_en[first].alone) {
    sprintf(out, "%s WITH %sS", name, weapon);
    g_title_font.Measure(&size, out);
    if (size.cx >= 240) {
      out[strlen(name)] = '\n';
    }
  } else {
    sprintf(out, "%s%s %s", weapon,
            weapon[strlen(weapon) - 1] == 'E' ? "D" : "ED", name);
    g_title_font.Measure(&size, out);
    if (size.cx >= 240) {
      out[strlen(out) - strlen(name) - 1] = '\n';
    }
  }
  if (state_.title.variant >= 0) {
    const char* variant = g_variant_words_en[state_.title.variant];
    strcat(out, variant);
    g_title_font.Measure(&size, out);
    if (size.cx >= 240) {
      out[strlen(out) - strlen(variant)] = '\n';
    }
  }
}
