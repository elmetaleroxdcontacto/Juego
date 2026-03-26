#include "engine/front_menu.h"

#include "utils/utils.h"

#include <iostream>

using namespace std;

MainMenuScreen::MainMenuScreen(const GameSettings& settings) : settings_(settings) {}

void MainMenuScreen::render() const {
    cout << "\n====================================" << endl;
    cout << "      " << game_settings::gameTitle() << endl;
    cout << "====================================" << endl;
    cout << "1. Jugar" << endl;
    cout << "2. Configuraciones" << endl;
    cout << "3. Salir" << endl;
    cout << "\nPerfil actual: " << game_settings::settingsSummary(settings_) << endl;
}

FrontMenuAction MainMenuScreen::prompt() const {
    render();
    const int choice = readInt("Elige una opcion: ", 1, 3);
    if (choice == 1) return FrontMenuAction::Play;
    if (choice == 2) return FrontMenuAction::Settings;
    return FrontMenuAction::Exit;
}

SettingsMenuScreen::SettingsMenuScreen(GameSettings& settings) : settings_(settings) {}

void SettingsMenuScreen::render() const {
    cout << "\n=== Configuraciones ===" << endl;
    cout << "1. Volumen: " << game_settings::volumeLabel(settings_.volume) << endl;
    cout << "2. Dificultad: " << game_settings::difficultyLabel(settings_.difficulty) << endl;
    cout << "   " << game_settings::difficultyDescription(settings_.difficulty) << endl;
    cout << "3. Modo de simulacion: " << game_settings::simulationModeLabel(settings_.simulationMode) << endl;
    cout << "   " << game_settings::simulationModeDescription(settings_.simulationMode) << endl;
    cout << "4. Volver" << endl;
}

void SettingsMenuScreen::run() {
    while (true) {
        render();
        const int choice = readInt("Ajuste a modificar: ", 1, 4);
        if (choice == 4) return;

        if (choice == 1) {
            game_settings::cycleVolume(settings_);
            cout << "Volumen actualizado a " << game_settings::volumeLabel(settings_.volume) << "." << endl;
        } else if (choice == 2) {
            game_settings::cycleDifficulty(settings_);
            cout << "Dificultad actual: " << game_settings::difficultyLabel(settings_.difficulty) << "." << endl;
        } else if (choice == 3) {
            game_settings::cycleSimulationMode(settings_);
            cout << "Modo de simulacion actual: " << game_settings::simulationModeLabel(settings_.simulationMode) << "." << endl;
        }
    }
}
