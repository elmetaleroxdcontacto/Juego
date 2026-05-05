#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

enum class GameDifficulty {
    Accessible,
    Normal,
    Challenging,
    Realistic
};

enum class SimulationMode {
    Fast,
    Detailed
};

enum class SimulationSpeed {
    Relaxed,
    Standard,
    Rapid
};

enum class UiLanguage {
    Spanish,
    English
};

enum class TextSpeed {
    Relaxed,
    Standard,
    Rapid
};

enum class VisualProfile {
    Broadcast,
    Compact,
    HighContrast
};

enum class MenuMusicMode {
    Off,
    MainMenuOnly,
    FrontendPages
};

struct GameSettings {
    int volume = 70;
    GameDifficulty difficulty = GameDifficulty::Normal;
    SimulationSpeed simulationSpeed = SimulationSpeed::Standard;
    SimulationMode simulationMode = SimulationMode::Detailed;
    UiLanguage language = UiLanguage::Spanish;
    TextSpeed textSpeed = TextSpeed::Standard;
    VisualProfile visualProfile = VisualProfile::Broadcast;
    MenuMusicMode menuMusicMode = MenuMusicMode::MainMenuOnly;
    bool menuAudioFade = true;
    std::string menuThemeId = "el-crack";
};

namespace game_settings {

std::string gameTitle();
std::string settingsFilePath();
int clampVolume(int value);
int nextVolume(int current);
GameDifficulty nextDifficulty(GameDifficulty current);
SimulationSpeed nextSimulationSpeed(SimulationSpeed current);
SimulationMode nextSimulationMode(SimulationMode current);
UiLanguage nextLanguage(UiLanguage current);
TextSpeed nextTextSpeed(TextSpeed current);
VisualProfile nextVisualProfile(VisualProfile current);
MenuMusicMode nextMenuMusicMode(MenuMusicMode current);
std::string volumeLabel(int value);
std::string difficultyLabel(GameDifficulty difficulty);
std::string difficultyDescription(GameDifficulty difficulty);
std::string simulationSpeedLabel(SimulationSpeed speed);
std::string simulationSpeedDescription(SimulationSpeed speed);
std::string simulationModeLabel(SimulationMode mode);
std::string simulationModeDescription(SimulationMode mode);
std::string languageLabel(UiLanguage language);
std::string languageDescription(UiLanguage language);
std::string textSpeedLabel(TextSpeed speed);
std::string textSpeedDescription(TextSpeed speed);
std::string visualProfileLabel(VisualProfile profile);
std::string visualProfileDescription(VisualProfile profile);
std::string menuMusicModeLabel(MenuMusicMode mode);
std::string menuMusicModeDescription(MenuMusicMode mode);
std::string menuAudioFadeLabel(bool enabled);
std::string menuAudioFadeDescription(bool enabled);
std::string menuThemeLabel(const GameSettings& settings);
std::string settingsSummary(const GameSettings& settings);
std::vector<std::string> detailedSettingsSummary(const GameSettings& settings);
bool isDetailedSimulation(const GameSettings& settings);
bool shouldPlayMenuMusic(const GameSettings& settings, bool onMainMenu, bool onFrontendPage);
bool sanitize(GameSettings& settings);
bool loadFromDisk(GameSettings& settings, const std::string& path = std::string());
bool saveToDisk(const GameSettings& settings, const std::string& path = std::string());
int uiPulseDelayMs(const GameSettings& settings);
int pageTransitionDelayMs(const GameSettings& settings);
int audioFadeStepDelayMs(const GameSettings& settings);
void cycleVolume(GameSettings& settings);
void cycleDifficulty(GameSettings& settings);
void cycleSimulationSpeed(GameSettings& settings);
void cycleSimulationMode(GameSettings& settings);
void cycleLanguage(GameSettings& settings);
void cycleTextSpeed(GameSettings& settings);
void cycleVisualProfile(GameSettings& settings);
void cycleMenuMusicMode(GameSettings& settings);
void toggleMenuAudioFade(GameSettings& settings);
void applyNewCareerDifficulty(Career& career, const GameSettings& settings);
void applyQuickMatchDifficulty(Team& userTeam, Team& opponentTeam, const GameSettings& settings);

}  // namespace game_settings
