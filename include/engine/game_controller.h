#pragma once

#include "engine/game_engine.h"

#include <cstddef>

class GameController {
public:
    int run(int argc, char* argv[]);

private:
    void showLoadWarnings() const;
    int runConsoleApp();
    void runCareerMode();
    void runQuickGame();
    void pauseForContinue() const;

    GameEngine engine_;
    mutable std::size_t lastLoadWarningSignature_ = 0;
};
