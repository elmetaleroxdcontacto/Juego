#pragma once

#include "engine/game_engine.h"
#include "engine/game_settings.h"

#include <cstddef>

class GameController {
public:
    int run(int argc, char* argv[]);

private:
    void showLoadWarnings() const;
    int runConsoleApp();
    void runConsoleGameHub();
    void runCareerMode();
    void runQuickGame();
    void pauseForContinue() const;

    GameEngine engine_;
    GameSettings settings_;
    mutable std::size_t lastLoadWarningSignature_ = 0;
};
