// fnTitle: the title screen.
#include "fn_title.h"

#include "actor_system.h"
#include "application.h"
#include "hls_txt.h"

// 0x41df30
FnTitle::FnTitle() {
  name_ = "fnTitle";
  texts_[0] = texts_[1] = texts_[2] = NULL;
  unknown_50_ = 0;
}

// 0x41df70
void FnTitle::Finalize() {
  g_actor_system.DestroyAll();
  if (texts_[0] != NULL) {
    texts_[0]->Release();
    texts_[0] = NULL;
  }
  if (texts_[1] != NULL) {
    texts_[1]->Release();
    texts_[1] = NULL;
  }
  if (texts_[2] != NULL) {
    texts_[2]->Release();
    texts_[2] = NULL;
  }
  g_application.tasks.Delete(g_title_background);
  g_title_background = NULL;
}
