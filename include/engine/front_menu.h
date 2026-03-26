#pragma once

#include "engine/game_settings.h"

enum class FrontMenuAction {
    Play,
    Settings,
    Exit
};

class MenuScreen {
public:
    virtual ~MenuScreen() = default;
    virtual void render() const = 0;
};

class MainMenuScreen final : public MenuScreen {
public:
    explicit MainMenuScreen(const GameSettings& settings);

    void render() const override;
    FrontMenuAction prompt() const;

private:
    const GameSettings& settings_;
};

class SettingsMenuScreen final : public MenuScreen {
public:
    explicit SettingsMenuScreen(GameSettings& settings);

    void render() const override;
    void run();

private:
    GameSettings& settings_;
};
