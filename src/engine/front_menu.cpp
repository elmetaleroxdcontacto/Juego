#include "engine/front_menu.h"

#include "utils/utils.h"

#include <cctype>
#include <iostream>
#include <sstream>

using namespace std;

namespace {

constexpr int kConsolePanelWidth = 62;

string padOrTrim(const string& text, int width) {
    if (width <= 0) return string();
    if (static_cast<int>(text.size()) >= width) return text.substr(0, static_cast<size_t>(width));
    return text + string(static_cast<size_t>(width - text.size()), ' ');
}

void printPanelBorder() {
    cout << "+" << string(static_cast<size_t>(kConsolePanelWidth - 2), '-') << "+" << endl;
}

void printPanelLine(const string& text = string()) {
    cout << "| " << padOrTrim(text, kConsolePanelWidth - 4) << " |" << endl;
}

void renderPanelBlock(const string& title, const vector<string>& lines) {
    printPanelBorder();
    printPanelLine(title);
    printPanelBorder();
    if (lines.empty()) {
        printPanelLine();
    } else {
        for (const auto& line : lines) {
            printPanelLine(line);
        }
    }
    printPanelBorder();
}

string normalizeMenuInput(const string& value) {
    string normalized = trim(value);
    for (char& ch : normalized) {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }
    return normalized;
}

string uppercaseCopy(const string& value) {
    string out = value;
    for (char& ch : out) {
        ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    }
    return out;
}

string actionName(FrontMenuAction action) {
    switch (action) {
        case FrontMenuAction::CycleVolume: return "Volumen";
        case FrontMenuAction::CycleDifficulty: return "Dificultad";
        case FrontMenuAction::CycleSimulationSpeed: return "Velocidad de simulacion";
        case FrontMenuAction::CycleSimulationMode: return "Modo de simulacion";
        default: return "Ajuste";
    }
}

}  // namespace

MainMenuScreen::MainMenuScreen(const GameSettings& settings) : settings_(settings) {}

string MainMenuScreen::headline() const {
    return game_settings::gameTitle();
}

string MainMenuScreen::sectionTitle() const {
    return "Portada del manager";
}

string MainMenuScreen::subtitle() const {
    return "Portada inicial inspirada en un simulador de gestion futbolistica: limpia, directa y lista para crecer.";
}

string MainMenuScreen::helperText() const {
    return "Navegacion: W/S o arriba/abajo, Enter confirma, numeros y atajos. Q sale al escritorio.";
}

vector<string> MainMenuScreen::statusLines() const {
    return {
        "Perfil actual: " + game_settings::settingsSummary(settings_),
        "Audio: placeholder listo para integracion futura.",
        "Entrada recomendada: Jugar abre el flujo real del juego."
    };
}

vector<string> MainMenuScreen::roadmapLines() const {
    return {
        "Base preparada para Continuar, Nueva partida y Cargar.",
        "La GUI comparte los mismos ajustes iniciales del frontend.",
        "F11 y fullscreen siguen disponibles al entrar en la interfaz Win32."
    };
}

vector<MenuOption> MainMenuScreen::options() const {
    return {
        {"Jugar",
         "Abre el flujo principal existente: dashboard, carrera, fichajes, tacticas y centro del club.",
         FrontMenuAction::Play,
         'j',
         true},
        {"Configuraciones",
         "Ajusta volumen, dificultad, velocidad de simulacion y modo de simulacion antes de entrar.",
         FrontMenuAction::Settings,
         'c',
         false}
    };
}

SettingsMenuScreen::SettingsMenuScreen(const GameSettings& settings) : settings_(settings) {}

string SettingsMenuScreen::headline() const {
    return game_settings::gameTitle();
}

string SettingsMenuScreen::sectionTitle() const {
    return "Cabina de configuracion";
}

string SettingsMenuScreen::subtitle() const {
    return "Ajustes base compartidos entre consola y GUI, con estructura lista para audio, idioma y video.";
}

string SettingsMenuScreen::helperText() const {
    return "Navegacion: W/S, Enter, numeros y Q para volver al menu principal.";
}

vector<string> SettingsMenuScreen::statusLines() const {
    return {
        "Volumen: " + game_settings::volumeLabel(settings_.volume),
        "Dificultad: " + game_settings::difficultyLabel(settings_.difficulty),
        "Velocidad: " + game_settings::simulationSpeedLabel(settings_.simulationSpeed),
        "Simulacion: " + game_settings::simulationModeLabel(settings_.simulationMode)
    };
}

vector<string> SettingsMenuScreen::roadmapLines() const {
    return {
        "Audio: canal preparado para integracion futura.",
        "Video: pendiente para resolucion, ventana y fullscreen.",
        "Persistencia: lista para guardar configuraciones en disco."
    };
}

vector<MenuOption> SettingsMenuScreen::options() const {
    return {
        {"Volumen: " + game_settings::volumeLabel(settings_.volume),
         "Control de frontend listo para conectarse a audio real cuando exista.",
         FrontMenuAction::CycleVolume,
         'v',
         true},
        {"Dificultad: " + game_settings::difficultyLabel(settings_.difficulty),
         game_settings::difficultyDescription(settings_.difficulty),
         FrontMenuAction::CycleDifficulty,
         'd',
         false},
        {"Velocidad de simulacion: " + game_settings::simulationSpeedLabel(settings_.simulationSpeed),
         game_settings::simulationSpeedDescription(settings_.simulationSpeed),
         FrontMenuAction::CycleSimulationSpeed,
         'r',
         false},
        {"Modo de simulacion: " + game_settings::simulationModeLabel(settings_.simulationMode),
         game_settings::simulationModeDescription(settings_.simulationMode),
         FrontMenuAction::CycleSimulationMode,
         'm',
         false},
        {"Volver",
         "Regresa al menu principal sin perder cambios.",
         FrontMenuAction::Back,
         'q',
         false}
    };
}

MenuRenderer::MenuRenderer(const GameSettings& settings) : settings_(settings) {}

void MenuRenderer::renderHeadline(const MenuScreen& screen) const {
    cout << "\n";
    printPanelBorder();
    printPanelLine(uppercaseCopy(screen.headline()));
    printPanelLine(screen.sectionTitle());
    printPanelLine(screen.subtitle());
    printPanelBorder();
}

void MenuRenderer::renderOptionsPanel(const vector<MenuOption>& options, int selectedIndex) const {
    vector<string> lines;
    for (size_t i = 0; i < options.size(); ++i) {
        ostringstream label;
        label << (static_cast<int>(i) == selectedIndex ? ">> " : "   ");
        label << "[" << i + 1 << "] " << options[i].label;
        if (options[i].primary) label << " <principal>";
        lines.push_back(label.str());
        lines.push_back("    " + options[i].description);
        if (i + 1 != options.size()) lines.push_back(string());
    }
    renderPanelBlock("Panel central", lines);
}

void MenuRenderer::renderInfoPanel(const string& title, const vector<string>& lines) const {
    renderPanelBlock(title, lines);
}

void MenuRenderer::render(const MenuScreen& screen, int selectedIndex, bool allowExitShortcut) const {
    renderHeadline(screen);
    renderInfoPanel("Estado de sesion", screen.statusLines());
    renderOptionsPanel(screen.options(), selectedIndex);
    renderInfoPanel("Hoja de ruta", screen.roadmapLines());

    vector<string> helperLines;
    helperLines.push_back(screen.helperText());
    helperLines.push_back("Resumen rapido: " + game_settings::settingsSummary(settings_));
    if (allowExitShortcut) {
        helperLines.push_back("Q tambien cierra el juego desde esta pantalla.");
    }
    renderInfoPanel("Controles", helperLines);
}

MenuActionHandler::MenuActionHandler(GameSettings& settings) : settings_(settings) {}

void MenuActionHandler::applySettingsAction(FrontMenuAction action) const {
    switch (action) {
        case FrontMenuAction::CycleVolume:
            game_settings::cycleVolume(settings_);
            return;
        case FrontMenuAction::CycleDifficulty:
            game_settings::cycleDifficulty(settings_);
            return;
        case FrontMenuAction::CycleSimulationSpeed:
            game_settings::cycleSimulationSpeed(settings_);
            return;
        case FrontMenuAction::CycleSimulationMode:
            game_settings::cycleSimulationMode(settings_);
            return;
        default:
            return;
    }
}

void MenuActionHandler::printSettingsFeedback(FrontMenuAction action) const {
    switch (action) {
        case FrontMenuAction::CycleVolume:
            cout << actionName(action) << " actualizado a "
                 << game_settings::volumeLabel(settings_.volume) << "." << endl;
            return;
        case FrontMenuAction::CycleDifficulty:
            cout << actionName(action) << " actual: "
                 << game_settings::difficultyLabel(settings_.difficulty) << "." << endl;
            return;
        case FrontMenuAction::CycleSimulationSpeed:
            cout << actionName(action) << " actual: "
                 << game_settings::simulationSpeedLabel(settings_.simulationSpeed) << "." << endl;
            return;
        case FrontMenuAction::CycleSimulationMode:
            cout << actionName(action) << " actual: "
                 << game_settings::simulationModeLabel(settings_.simulationMode) << "." << endl;
            return;
        default:
            return;
    }
}

MenuController::MenuController(GameSettings& settings)
    : settings_(settings), renderer_(settings), actionHandler_(settings) {}

bool MenuController::handleNavigationInput(const string& input,
                                           int& selectedIndex,
                                           size_t optionCount,
                                           bool allowExitShortcut,
                                           FrontMenuAction& action) const {
    action = FrontMenuAction::None;
    if (optionCount == 0) {
        action = allowExitShortcut ? FrontMenuAction::Exit : FrontMenuAction::Back;
        return true;
    }

    const string normalized = normalizeMenuInput(input);
    if (normalized.empty() || normalized == "e" || normalized == "ok") {
        return true;
    }
    if (normalized == "w" || normalized == "up" || normalized == "arriba") {
        selectedIndex = (selectedIndex + static_cast<int>(optionCount) - 1) % static_cast<int>(optionCount);
        return false;
    }
    if (normalized == "s" || normalized == "down" || normalized == "abajo") {
        selectedIndex = (selectedIndex + 1) % static_cast<int>(optionCount);
        return false;
    }
    if (normalized == "q" || normalized == "volver") {
        action = allowExitShortcut ? FrontMenuAction::Exit : FrontMenuAction::Back;
        return true;
    }
    if (normalized.size() == 1 && isdigit(static_cast<unsigned char>(normalized[0]))) {
        int numeric = normalized[0] - '1';
        if (numeric >= 0 && numeric < static_cast<int>(optionCount)) {
            selectedIndex = numeric;
            return true;
        }
    }

    return false;
}

FrontMenuAction MenuController::readScreenAction(const MenuScreen& screen, int& selectedIndex, bool allowExitShortcut) const {
    const vector<MenuOption> options = screen.options();
    while (true) {
        renderer_.render(screen, selectedIndex, allowExitShortcut);
        string input = readLine("Seleccion: ");
        FrontMenuAction action = FrontMenuAction::None;
        if (!handleNavigationInput(input, selectedIndex, options.size(), allowExitShortcut, action)) {
            const string normalized = normalizeMenuInput(input);
            bool matchedHotkey = false;
            for (size_t i = 0; i < options.size(); ++i) {
                if (options[i].hotkey == '\0') continue;
                if (normalized.size() == 1 && normalized[0] == options[i].hotkey) {
                    selectedIndex = static_cast<int>(i);
                    matchedHotkey = true;
                    break;
                }
            }
            if (matchedHotkey) {
                return options[static_cast<size_t>(selectedIndex)].action;
            }
            cout << "Entrada no valida. Usa W/S, Enter, numeros";
            if (!allowExitShortcut) cout << " o Q para volver";
            else cout << " o Q para salir";
            cout << "." << endl;
            continue;
        }

        if (action != FrontMenuAction::None) return action;
        return options[static_cast<size_t>(selectedIndex)].action;
    }
}

FrontMenuAction MenuController::runMainMenu() {
    int selectedIndex = 0;
    return readScreenAction(MainMenuScreen(settings_), selectedIndex, true);
}

void MenuController::runSettingsMenu() {
    int selectedIndex = 0;
    while (true) {
        FrontMenuAction action = readScreenAction(SettingsMenuScreen(settings_), selectedIndex, false);
        if (action == FrontMenuAction::Back || action == FrontMenuAction::Exit) {
            cout << "Volviendo al menu principal." << endl;
            return;
        }
        actionHandler_.applySettingsAction(action);
        actionHandler_.printSettingsFeedback(action);
    }
}
