#include "engine/game_controller.h"
#include "utils/logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleTitleA("Chilean Footballito");
#endif

    Logger::getInstance().setLogFile("football_manager.log");
    LOG_INFO("Iniciando Chilean Footballito");

    GameController controller;
    return controller.run(argc, argv);
}
