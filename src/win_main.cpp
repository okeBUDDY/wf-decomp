// WinMain: the start-up settings, the systems, and the main loop. The
// original compiles it with the definitions of g_application and g_pat_sys.
#include <windows.h>

#include "actor_system.h"
#include "application.h"
#include "cglobal.h"
#include "enm_param.h"
#include "fn_title.h"
#include "hl_3d.h"
#include "hl_input.h"
#include "hl_sync.h"
#include "hl_window.h"
#include "hls_anm.h"
#include "hls_pat.h"
#include "user_func.h"

namespace {

const char kTitle[] = "WARNING FOREVER v1.07";
const char kName[] = "wf";
const int kIcon = 103;

}  // namespace

// 0x401fc0
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line,
                   int show) {
  g_application.instance = instance;
  g_application.graphics_flags = AppSettings::kDirectDraw |
                                 AppSettings::kDirect3D | AppSettings::kInput |
                                 AppSettings::kSound;
  g_application.options = AppSettings::kDeviceDialog |
                          AppSettings::kScreenChoice |
                          AppSettings::kSystemKeys | AppSettings::kHideCursor;
  g_application.width = 640;
  g_application.height = 480;
  g_application.depth = 16;
  g_application.sound_channels = 40;
  g_application.flags = AppSettings::kFullScreen;
  g_application.window_system->set_icon(kIcon);
  g_application.d3d->set_depth(0);
  g_application.input->set_pad_count(1);
  g_application.input->set_max_buttons(8);
  g_application.sync->set_mode(HLSync::kWait);
  g_application.sync->set_fps(60);
  g_pat_sys.set_count(0x100);
  g_anm_sys.set_count(0x200);
  g_actor_system.set_limits(0x80, 0x600, 8);
  if (!g_application.Initialize(kTitle, kName)) {
    g_application.Quit();
  }
  if (!g_application.AddSystem(&g_pat_sys, false)) {
    g_application.Quit();
  }
  if (!g_application.AddSystem(&g_anm_sys, false)) {
    g_application.Quit();
  }
  if (!g_application.AddSystem(&g_actor_system, false)) {
    g_application.Quit();
  }
  if (!g_application.AddSystem(&g_global, false)) {
    g_application.Quit();
  }
  if (!g_application.AddSystem(&g_enm_param, false)) {
    g_application.Quit();
  }
  g_application.reset_func = &g_fn_title;
  g_application.user_func->Finish(&g_fn_title);
  return g_application.Run();
}
