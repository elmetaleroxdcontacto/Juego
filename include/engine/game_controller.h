#pragma once

#include "engine/game_engine.h"
#include "engine/game_settings.h"

#include <cstddef>

class GameController {
public:
    int run(int argc, char* argv[]);

private:
    void showLoadWarnings() const;
    void loadPersistedSettings();
    void savePersistedSettings() const;
    int runConsoleApp();
    void runConsoleGameHub();
    void runCareerMode();
    void runCareerLoop();
    void runQuickGame();
    bool loadCareerForResume(bool forceReloadFromDisk);
    bool createNewCareerFromConsole();
    void pauseForContinue() const;

    GameEngine engine_;
    GameSettings settings_;
    mutable std::size_t lastLoadWarningSignature_ = 0;
};
