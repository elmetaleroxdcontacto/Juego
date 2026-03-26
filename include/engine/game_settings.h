#pragma once

#include "engine/models.h"

#include <string>

enum class GameDifficulty {
    Accessible,
    Normal,
    Challenging
};

enum class SimulationMode {
    Fast,
    Detailed
};

struct GameSettings {
    int volume = 70;
    GameDifficulty difficulty = GameDifficulty::Normal;
    SimulationMode simulationMode = SimulationMode::Detailed;
};

namespace game_settings {

std::string gameTitle();
int clampVolume(int value);
int nextVolume(int current);
GameDifficulty nextDifficulty(GameDifficulty current);
SimulationMode nextSimulationMode(SimulationMode current);
std::string volumeLabel(int value);
std::string difficultyLabel(GameDifficulty difficulty);
std::string difficultyDescription(GameDifficulty difficulty);
std::string simulationModeLabel(SimulationMode mode);
std::string simulationModeDescription(SimulationMode mode);
std::string settingsSummary(const GameSettings& settings);
bool isDetailedSimulation(const GameSettings& settings);
void cycleVolume(GameSettings& settings);
void cycleDifficulty(GameSettings& settings);
void cycleSimulationMode(GameSettings& settings);
void applyNewCareerDifficulty(Career& career, const GameSettings& settings);
void applyQuickMatchDifficulty(Team& userTeam, Team& opponentTeam, const GameSettings& settings);

}  // namespace game_settings
