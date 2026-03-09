#pragma once

#include "engine/game_engine.h"

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
};
