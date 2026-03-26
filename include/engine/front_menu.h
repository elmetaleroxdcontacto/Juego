#pragma once

#include "engine/game_settings.h"
#include "engine/models.h"

#include <string>
#include <vector>

enum class FrontMenuAction {
    None,
    ContinueCareer,
    Play,
    LoadCareer,
    Settings,
    Credits,
    CycleVolume,
    CycleDifficulty,
    CycleSimulationSpeed,
    CycleSimulationMode,
    CycleLanguage,
    CycleTextSpeed,
    CycleVisualProfile,
    CycleMenuMusicMode,
    ToggleMenuAudioFade,
    Back,
    Exit
};

struct MenuOption {
    std::string label;
    std::string description;
    FrontMenuAction action = FrontMenuAction::None;
    char hotkey = '\0';
    bool primary = false;
};

class MenuScreen {
public:
    virtual ~MenuScreen() = default;
    virtual std::string headline() const = 0;
    virtual std::string sectionTitle() const = 0;
    virtual std::string subtitle() const = 0;
    virtual std::string helperText() const = 0;
    virtual std::vector<std::string> statusLines() const = 0;
    virtual std::vector<std::string> roadmapLines() const = 0;
    virtual std::vector<MenuOption> options() const = 0;
};

class MainMenuScreen final : public MenuScreen {
public:
    MainMenuScreen(const GameSettings& settings, const Career& career);

    std::string headline() const override;
    std::string sectionTitle() const override;
    std::string subtitle() const override;
    std::string helperText() const override;
    std::vector<std::string> statusLines() const override;
    std::vector<std::string> roadmapLines() const override;
    std::vector<MenuOption> options() const override;

private:
    const GameSettings& settings_;
    const Career& career_;
};

class SettingsMenuScreen final : public MenuScreen {
public:
    explicit SettingsMenuScreen(const GameSettings& settings);

    std::string headline() const override;
    std::string sectionTitle() const override;
    std::string subtitle() const override;
    std::string helperText() const override;
    std::vector<std::string> statusLines() const override;
    std::vector<std::string> roadmapLines() const override;
    std::vector<MenuOption> options() const override;

private:
    const GameSettings& settings_;
};

class CreditsMenuScreen final : public MenuScreen {
public:
    explicit CreditsMenuScreen(const GameSettings& settings);

    std::string headline() const override;
    std::string sectionTitle() const override;
    std::string subtitle() const override;
    std::string helperText() const override;
    std::vector<std::string> statusLines() const override;
    std::vector<std::string> roadmapLines() const override;
    std::vector<MenuOption> options() const override;

private:
    const GameSettings& settings_;
};

class MenuRenderer {
public:
    explicit MenuRenderer(const GameSettings& settings);

    void render(const MenuScreen& screen, int selectedIndex, bool allowExitShortcut) const;

private:
    void renderHeadline(const MenuScreen& screen) const;
    void renderOptionsPanel(const std::vector<MenuOption>& options, int selectedIndex) const;
    void renderInfoPanel(const std::string& title, const std::vector<std::string>& lines) const;

    const GameSettings& settings_;
};

class MenuActionHandler {
public:
    explicit MenuActionHandler(GameSettings& settings);

    void applySettingsAction(FrontMenuAction action) const;
    void printSettingsFeedback(FrontMenuAction action) const;

private:
    GameSettings& settings_;
};

class MenuController {
public:
    MenuController(GameSettings& settings, Career& career);

    FrontMenuAction runMainMenu();
    void runSettingsMenu();
    void runCreditsMenu();

private:
    FrontMenuAction readScreenAction(const MenuScreen& screen, int& selectedIndex, bool allowExitShortcut) const;
    bool handleNavigationInput(const std::string& input,
                               int& selectedIndex,
                               std::size_t optionCount,
                               bool allowExitShortcut,
                               FrontMenuAction& action) const;

    GameSettings& settings_;
    Career& career_;
    MenuRenderer renderer_;
    MenuActionHandler actionHandler_;
};
