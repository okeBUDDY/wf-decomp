// fnOption: the option screen.
#include "fn_option.h"

#include <string.h>

#include "application.h"
#include "background.h"
#include "hls_txt.h"

// 0x41c6e0
FnOption::FnOption() {
  name_ = "fnOption";
  for (int i = 0; i < kItems; ++i) {
    items_[i] = NULL;
    enabled_[i] = true;
  }
  background_ = NULL;
}

// 0x41c8a0
// TODO: nonmatching: VC unrolls the loop, which the original keeps.
void FnOption::SetPreset(int preset) {
  for (int i = 0; i < kRules; ++i) {
    enabled_[kFirstRuleItem + i] = preset == kCustomPreset;
    rules_[i] = presets_[preset][i];
  }
}

// 0x41ca50
void HLSTxt::SetText(const char* text) { strcpy(text_, text); }

// 0x41ded0
void FnOption::Finalize() {
  Apply();
  SaveSettings();
  for (int i = 0; i < kItems; ++i) {
    if (items_[i] != NULL) {
      items_[i]->Release();
      items_[i] = NULL;
    }
  }
  if (background_ != NULL) {
    g_application.tasks.Delete(background_);
    background_ = NULL;
  }
}
