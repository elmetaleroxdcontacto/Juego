#include "engine/front_menu.h"

#include "utils/utils.h"

#include <cctype>
#include <iostream>
#include <sstream>

using namespace std;

namespace {

constexpr int kConsolePanelWidth = 70;

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
        case FrontMenuAction::CycleLanguage: return "Idioma";
        case FrontMenuAction::CycleTextSpeed: return "Velocidad de texto";
        case FrontMenuAction::CycleVisualProfile: return "Perfil visual";
        case FrontMenuAction::CycleMenuMusicMode: return "Musica del frontend";
        case FrontMenuAction::ToggleMenuAudioFade: return "Transicion de audio";
        default: return "Ajuste";
    }
}

bool hasSavedCareer(const Career& career) {
    string savePath = career.saveFile.empty() ? "saves/career_save.txt" : career.saveFile;
    return pathExists(savePath) || (savePath == "saves/career_save.txt" && pathExists("career_save.txt"));
}

}  // namespace

MainMenuScreen::MainMenuScreen(const GameSettings& settings, const Career& career)
    : settings_(settings), career_(career) {}

string MainMenuScreen::headline() const {
    return game_settings::gameTitle();
}

string MainMenuScreen::sectionTitle() const {
    return "Portada del manager";
}

string MainMenuScreen::subtitle() const {
    return "Frontend principal inspirado en un manager game: continuidad, configuracion y acceso directo al juego real.";
}

string MainMenuScreen::helperText() const {
    return "Navegacion: W/S o arriba/abajo, Enter confirma, numeros y atajos. Q sale al escritorio.";
}

vector<string> MainMenuScreen::statusLines() const {
    const bool hasSave = hasSavedCareer(career_);
    const bool hasInMemoryCareer = career_.myTeam != nullptr;
    return {
        "Perfil actual: " + game_settings::settingsSummary(settings_),
        string("Continuar: ") + (hasInMemoryCareer ? "sesion activa en memoria" : (hasSave ? "guardado disponible" : "sin guardado")),
        "Musica frontend: " + game_settings::menuMusicModeLabel(settings_.menuMusicMode),
        "Visual: " + game_settings::visualProfileLabel(settings_.visualProfile)
    };
}

vector<string> MainMenuScreen::roadmapLines() const {
    return {
        "Continuar y Cargar ya entran al flujo real sin crear rutas paralelas.",
        "Configuraciones se persisten en disco y alimentan CLI, GUI y audio.",
        "La arquitectura queda lista para perfil, idioma, video, resolucion y mas submenus."
    };
}

vector<MenuOption> MainMenuScreen::options() const {
    return {
        {"Continuar",
         "Retoma la carrera activa o intenta cargar el ultimo guardado directo al flujo real.",
         FrontMenuAction::ContinueCareer,
         't',
         true},
        {"Jugar",
         "Abre el hub principal existente: carrera, juego rapido, copa y validacion.",
         FrontMenuAction::Play,
         'j',
         false},
        {"Cargar guardado",
         "Fuerza la carga del ultimo save y entra al proyecto actual del club.",
         FrontMenuAction::LoadCareer,
         'l',
         false},
        {"Configuraciones",
         "Ajusta audio, dificultad, velocidad, idioma y preferencias visuales antes de entrar.",
         FrontMenuAction::Settings,
         'c',
         false},
        {"Creditos",
         "Muestra identidad del proyecto, tecnologia y direccion actual del simulador.",
         FrontMenuAction::Credits,
         'r',
         false},
        {"Salir",
         "Cierra el juego y guarda el ultimo perfil de configuracion en disco.",
         FrontMenuAction::Exit,
         'q',
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
    return "Ajustes persistentes compartidos entre consola, GUI, audio del frontend y timing de interfaz.";
}

string SettingsMenuScreen::helperText() const {
    return "Navegacion: W/S, Enter, numeros y Q para volver al menu principal.";
}

vector<string> SettingsMenuScreen::statusLines() const {
    return game_settings::detailedSettingsSummary(settings_);
}

vector<string> SettingsMenuScreen::roadmapLines() const {
    return {
        "La velocidad de simulacion ya modifica pulsos y transiciones del frontend.",
        "El idioma queda preparado para localizacion futura sin romper el flujo actual.",
        "El modo de musica define si el tema vive solo en portada o en todo el frontend."
    };
}

vector<MenuOption> SettingsMenuScreen::options() const {
    return {
        {"Volumen: " + game_settings::volumeLabel(settings_.volume),
         "Control de salida general del frontend y referencia para la musica del menu.",
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
         's',
         false},
        {"Modo de simulacion: " + game_settings::simulationModeLabel(settings_.simulationMode),
         game_settings::simulationModeDescription(settings_.simulationMode),
         FrontMenuAction::CycleSimulationMode,
         'm',
         false},
        {"Idioma: " + game_settings::languageLabel(settings_.language),
         game_settings::languageDescription(settings_.language),
         FrontMenuAction::CycleLanguage,
         'i',
         false},
        {"Velocidad de texto: " + game_settings::textSpeedLabel(settings_.textSpeed),
         game_settings::textSpeedDescription(settings_.textSpeed),
         FrontMenuAction::CycleTextSpeed,
         't',
         false},
        {"Perfil visual: " + game_settings::visualProfileLabel(settings_.visualProfile),
         game_settings::visualProfileDescription(settings_.visualProfile),
         FrontMenuAction::CycleVisualProfile,
         'p',
         false},
        {"Musica del frontend: " + game_settings::menuMusicModeLabel(settings_.menuMusicMode),
         game_settings::menuMusicModeDescription(settings_.menuMusicMode),
         FrontMenuAction::CycleMenuMusicMode,
         'u',
         false},
        {"Transicion de audio: " + game_settings::menuAudioFadeLabel(settings_.menuAudioFade),
         game_settings::menuAudioFadeDescription(settings_.menuAudioFade),
         FrontMenuAction::ToggleMenuAudioFade,
         'f',
         false},
        {"Volver",
         "Regresa al menu principal sin perder cambios ni el perfil persistido.",
         FrontMenuAction::Back,
         'q',
         false}
    };
}

CreditsMenuScreen::CreditsMenuScreen(const GameSettings& settings) : settings_(settings) {}

string CreditsMenuScreen::headline() const {
    return game_settings::gameTitle();
}

string CreditsMenuScreen::sectionTitle() const {
    return "Creditos";
}

string CreditsMenuScreen::subtitle() const {
    return "Proyecto C++ con GUI Win32 y una base de simulador de gestion futbolistica construida para crecer.";
}

string CreditsMenuScreen::helperText() const {
    return "Enter o Q vuelven al menu principal.";
}

vector<string> CreditsMenuScreen::statusLines() const {
    return {
        "Motor: simulacion, carrera, scouting, staff, GUI Win32 y CLI compartida.",
        "Frontend: portada Chilean Footballito con audio, settings persistentes y accesos directos.",
        "Perfil actual: " + game_settings::settingsSummary(settings_)
    };
}

vector<string> CreditsMenuScreen::roadmapLines() const {
    return {
        "Objetivo: seguir acercando el proyecto a una experiencia tipo Football Manager.",
        "Base lista para nuevas pantallas: continuar, perfil, carga, creditos ampliados y opciones de video.",
        "El proyecto mantiene una arquitectura modular para evolucionar sin duplicar flujos."
    };
}

vector<MenuOption> CreditsMenuScreen::options() const {
    return {
        {"Volver",
         "Regresa al menu principal y conserva la misma configuracion persistida.",
         FrontMenuAction::Back,
         'q',
         true}
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
    bool changed = true;
    switch (action) {
        case FrontMenuAction::CycleVolume:
            game_settings::cycleVolume(settings_);
            break;
        case FrontMenuAction::CycleDifficulty:
            game_settings::cycleDifficulty(settings_);
            break;
        case FrontMenuAction::CycleSimulationSpeed:
            game_settings::cycleSimulationSpeed(settings_);
            break;
        case FrontMenuAction::CycleSimulationMode:
            game_settings::cycleSimulationMode(settings_);
            break;
        case FrontMenuAction::CycleLanguage:
            game_settings::cycleLanguage(settings_);
            break;
        case FrontMenuAction::CycleTextSpeed:
            game_settings::cycleTextSpeed(settings_);
            break;
        case FrontMenuAction::CycleVisualProfile:
            game_settings::cycleVisualProfile(settings_);
            break;
        case FrontMenuAction::CycleMenuMusicMode:
            game_settings::cycleMenuMusicMode(settings_);
            break;
        case FrontMenuAction::ToggleMenuAudioFade:
            game_settings::toggleMenuAudioFade(settings_);
            break;
        default:
            changed = false;
            break;
    }

    if (changed) {
        game_settings::saveToDisk(settings_);
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
        case FrontMenuAction::CycleLanguage:
            cout << actionName(action) << " actual: "
                 << game_settings::languageLabel(settings_.language) << "." << endl;
            return;
        case FrontMenuAction::CycleTextSpeed:
            cout << actionName(action) << " actual: "
                 << game_settings::textSpeedLabel(settings_.textSpeed) << "." << endl;
            return;
        case FrontMenuAction::CycleVisualProfile:
            cout << actionName(action) << " actual: "
                 << game_settings::visualProfileLabel(settings_.visualProfile) << "." << endl;
            return;
        case FrontMenuAction::CycleMenuMusicMode:
            cout << actionName(action) << " actual: "
                 << game_settings::menuMusicModeLabel(settings_.menuMusicMode) << "." << endl;
            return;
        case FrontMenuAction::ToggleMenuAudioFade:
            cout << actionName(action) << " actual: "
                 << game_settings::menuAudioFadeLabel(settings_.menuAudioFade) << "." << endl;
            return;
        default:
            return;
    }
}

MenuController::MenuController(GameSettings& settings, Career& career)
    : settings_(settings), career_(career), renderer_(settings), actionHandler_(settings) {}

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
    return readScreenAction(MainMenuScreen(settings_, career_), selectedIndex, true);
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

void MenuController::runCreditsMenu() {
    int selectedIndex = 0;
    while (true) {
        FrontMenuAction action = readScreenAction(CreditsMenuScreen(settings_), selectedIndex, false);
        if (action == FrontMenuAction::Back || action == FrontMenuAction::Exit) {
            cout << "Volviendo al menu principal." << endl;
            return;
        }
    }
}
