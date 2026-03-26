#pragma once

#ifdef _WIN32

#include "gui/gui_internal.h"

namespace gui_win32 {

void syncMenuMusicForPage(AppState& state);
void refreshMenuMusicVolume(AppState& state);
void shutdownMenuMusic(AppState& state);

}  // namespace gui_win32

#endif
