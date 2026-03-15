#include "engine/game_controller.h"

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    GameController controller;
    return controller.run(0, nullptr);
}
#endif
