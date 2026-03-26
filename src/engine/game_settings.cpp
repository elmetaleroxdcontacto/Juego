#include "engine/game_settings.h"

#include "utils/utils.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <sstream>

using namespace std;

namespace {

string normalizeSettingsPath(const string& path) {
    return trim(path.empty() ? string("saves/game_settings.cfg") : path);
}

int parseIntValue(const string& text, int defaultValue) {
    try {
        return stoi(trim(text));
    } catch (...) {
        return defaultValue;
    }
}

bool parseBoolValue(const string& text, bool defaultValue) {
    const string normalized = toLower(trim(text));
    if (normalized == "1" || normalized == "true" || normalized == "si" || normalized == "on") return true;
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") return false;
    return defaultValue;
}

string encodeBool(bool value) {
    return value ? "1" : "0";
}

string encodeDifficulty(GameDifficulty difficulty) {
    switch (difficulty) {
        case GameDifficulty::Accessible: return "accessible";
        case GameDifficulty::Normal: return "normal";
        case GameDifficulty::Challenging: return "challenging";
    }
    return "normal";
}

string encodeSimulationSpeed(SimulationSpeed speed) {
    switch (speed) {
        case SimulationSpeed::Relaxed: return "relaxed";
        case SimulationSpeed::Standard: return "standard";
        case SimulationSpeed::Rapid: return "rapid";
    }
    return "standard";
}

string encodeSimulationMode(SimulationMode mode) {
    return mode == SimulationMode::Fast ? "fast" : "detailed";
}

string encodeLanguage(UiLanguage language) {
    return language == UiLanguage::English ? "en" : "es";
}

string encodeTextSpeed(TextSpeed speed) {
    switch (speed) {
        case TextSpeed::Relaxed: return "relaxed";
        case TextSpeed::Standard: return "standard";
        case TextSpeed::Rapid: return "rapid";
    }
    return "standard";
}

string encodeVisualProfile(VisualProfile profile) {
    switch (profile) {
        case VisualProfile::Broadcast: return "broadcast";
        case VisualProfile::Compact: return "compact";
        case VisualProfile::HighContrast: return "high_contrast";
    }
    return "broadcast";
}

string encodeMenuMusicMode(MenuMusicMode mode) {
    switch (mode) {
        case MenuMusicMode::Off: return "off";
        case MenuMusicMode::MainMenuOnly: return "main_menu";
        case MenuMusicMode::FrontendPages: return "frontend_pages";
    }
    return "main_menu";
}

GameDifficulty parseDifficulty(const string& raw) {
    const string normalized = toLower(trim(raw));
    if (normalized == "accessible" || normalized == "accesible") return GameDifficulty::Accessible;
    if (normalized == "challenging" || normalized == "desafiante") return GameDifficulty::Challenging;
    return GameDifficulty::Normal;
}

SimulationSpeed parseSimulationSpeed(const string& raw) {
    const string normalized = toLower(trim(raw));
    if (normalized == "relaxed" || normalized == "pausada") return SimulationSpeed::Relaxed;
    if (normalized == "rapid" || normalized == "rapida") return SimulationSpeed::Rapid;
    return SimulationSpeed::Standard;
}

SimulationMode parseSimulationMode(const string& raw) {
    const string normalized = toLower(trim(raw));
    return (normalized == "fast" || normalized == "rapido") ? SimulationMode::Fast : SimulationMode::Detailed;
}

UiLanguage parseLanguage(const string& raw) {
    const string normalized = toLower(trim(raw));
    return (normalized == "en" || normalized == "english") ? UiLanguage::English : UiLanguage::Spanish;
}

TextSpeed parseTextSpeed(const string& raw) {
    const string normalized = toLower(trim(raw));
    if (normalized == "relaxed" || normalized == "pausada") return TextSpeed::Relaxed;
    if (normalized == "rapid" || normalized == "rapida") return TextSpeed::Rapid;
    return TextSpeed::Standard;
}

VisualProfile parseVisualProfile(const string& raw) {
    const string normalized = toLower(trim(raw));
    if (normalized == "compact" || normalized == "compacto") return VisualProfile::Compact;
    if (normalized == "high_contrast" || normalized == "high contrast" || normalized == "alto_contraste") {
        return VisualProfile::HighContrast;
    }
    return VisualProfile::Broadcast;
}

MenuMusicMode parseMenuMusicMode(const string& raw) {
    const string normalized = toLower(trim(raw));
    if (normalized == "off" || normalized == "apagado" || normalized == "silencio") return MenuMusicMode::Off;
    if (normalized == "frontend_pages" || normalized == "frontend") return MenuMusicMode::FrontendPages;
    return MenuMusicMode::MainMenuOnly;
}

int baseDelayFromSimulationSpeed(SimulationSpeed speed) {
    switch (speed) {
        case SimulationSpeed::Relaxed: return 170;
        case SimulationSpeed::Standard: return 95;
        case SimulationSpeed::Rapid: return 30;
    }
    return 95;
}

int textDelayModifier(TextSpeed speed) {
    switch (speed) {
        case TextSpeed::Relaxed: return 40;
        case TextSpeed::Standard: return 0;
        case TextSpeed::Rapid: return -20;
    }
    return 0;
}

}  // namespace

namespace game_settings {

string gameTitle() {
    return "Chilean Footballito";
}

string settingsFilePath() {
    return "saves/game_settings.cfg";
}

int clampVolume(int value) {
    return clampInt(value, 0, 100);
}

int nextVolume(int current) {
    static const array<int, 5> levels = {0, 25, 50, 75, 100};
    const int clamped = clampVolume(current);
    for (int level : levels) {
        if (clamped < level) return level;
    }
    return levels.front();
}

GameDifficulty nextDifficulty(GameDifficulty current) {
    switch (current) {
        case GameDifficulty::Accessible: return GameDifficulty::Normal;
        case GameDifficulty::Normal: return GameDifficulty::Challenging;
        case GameDifficulty::Challenging: return GameDifficulty::Accessible;
    }
    return GameDifficulty::Normal;
}

SimulationSpeed nextSimulationSpeed(SimulationSpeed current) {
    switch (current) {
        case SimulationSpeed::Relaxed: return SimulationSpeed::Standard;
        case SimulationSpeed::Standard: return SimulationSpeed::Rapid;
        case SimulationSpeed::Rapid: return SimulationSpeed::Relaxed;
    }
    return SimulationSpeed::Standard;
}

SimulationMode nextSimulationMode(SimulationMode current) {
    return current == SimulationMode::Fast ? SimulationMode::Detailed : SimulationMode::Fast;
}

UiLanguage nextLanguage(UiLanguage current) {
    return current == UiLanguage::Spanish ? UiLanguage::English : UiLanguage::Spanish;
}

TextSpeed nextTextSpeed(TextSpeed current) {
    switch (current) {
        case TextSpeed::Relaxed: return TextSpeed::Standard;
        case TextSpeed::Standard: return TextSpeed::Rapid;
        case TextSpeed::Rapid: return TextSpeed::Relaxed;
    }
    return TextSpeed::Standard;
}

VisualProfile nextVisualProfile(VisualProfile current) {
    switch (current) {
        case VisualProfile::Broadcast: return VisualProfile::Compact;
        case VisualProfile::Compact: return VisualProfile::HighContrast;
        case VisualProfile::HighContrast: return VisualProfile::Broadcast;
    }
    return VisualProfile::Broadcast;
}

MenuMusicMode nextMenuMusicMode(MenuMusicMode current) {
    switch (current) {
        case MenuMusicMode::Off: return MenuMusicMode::MainMenuOnly;
        case MenuMusicMode::MainMenuOnly: return MenuMusicMode::FrontendPages;
        case MenuMusicMode::FrontendPages: return MenuMusicMode::Off;
    }
    return MenuMusicMode::MainMenuOnly;
}

string volumeLabel(int value) {
    return to_string(clampVolume(value)) + "%";
}

string difficultyLabel(GameDifficulty difficulty) {
    switch (difficulty) {
        case GameDifficulty::Accessible: return "Accesible";
        case GameDifficulty::Normal: return "Normal";
        case GameDifficulty::Challenging: return "Desafiante";
    }
    return "Normal";
}

string difficultyDescription(GameDifficulty difficulty) {
    switch (difficulty) {
        case GameDifficulty::Accessible:
            return "Mas margen con la directiva y un arranque de proyecto mas amable.";
        case GameDifficulty::Normal:
            return "Balance estandar entre presion, presupuesto y exigencia competitiva.";
        case GameDifficulty::Challenging:
            return "Menos margen de error, menor aire economico y mas presion institucional.";
    }
    return "Balance estandar entre presion, presupuesto y exigencia competitiva.";
}

string simulationSpeedLabel(SimulationSpeed speed) {
    switch (speed) {
        case SimulationSpeed::Relaxed: return "Pausada";
        case SimulationSpeed::Standard: return "Normal";
        case SimulationSpeed::Rapid: return "Rapida";
    }
    return "Normal";
}

string simulationSpeedDescription(SimulationSpeed speed) {
    switch (speed) {
        case SimulationSpeed::Relaxed:
            return "Pensada para revisar decisiones con mas calma y contexto entre pasos.";
        case SimulationSpeed::Standard:
            return "Equilibrio entre ritmo de juego y lectura de reportes.";
        case SimulationSpeed::Rapid:
            return "Prioriza avanzar mas rapido por menus, cambios de pagina y simulaciones.";
    }
    return "Equilibrio entre ritmo de juego y lectura de reportes.";
}

string simulationModeLabel(SimulationMode mode) {
    switch (mode) {
        case SimulationMode::Fast: return "Rapido";
        case SimulationMode::Detailed: return "Detallado";
    }
    return "Detallado";
}

string simulationModeDescription(SimulationMode mode) {
    switch (mode) {
        case SimulationMode::Fast:
            return "Prioriza velocidad y menos salida visible durante las simulaciones.";
        case SimulationMode::Detailed:
            return "Mantiene mas contexto y trazas legibles para revisar cada decision.";
    }
    return "Mantiene mas contexto y trazas legibles para revisar cada decision.";
}

string languageLabel(UiLanguage language) {
    return language == UiLanguage::English ? "English" : "Espanol";
}

string languageDescription(UiLanguage language) {
    if (language == UiLanguage::English) {
        return "Base de localizacion lista para crecer hacia una interfaz bilingue.";
    }
    return "Idioma principal del proyecto. El resto del juego conserva compatibilidad futura con localizacion.";
}

string textSpeedLabel(TextSpeed speed) {
    switch (speed) {
        case TextSpeed::Relaxed: return "Pausado";
        case TextSpeed::Standard: return "Normal";
        case TextSpeed::Rapid: return "Agil";
    }
    return "Normal";
}

string textSpeedDescription(TextSpeed speed) {
    switch (speed) {
        case TextSpeed::Relaxed:
            return "Aumenta las pausas breves del frontend para una lectura mas calmada.";
        case TextSpeed::Standard:
            return "Balance natural entre lectura y respuesta del menu.";
        case TextSpeed::Rapid:
            return "Recorta pausas y transiciones para una experiencia mas agil.";
    }
    return "Balance natural entre lectura y respuesta del menu.";
}

string visualProfileLabel(VisualProfile profile) {
    switch (profile) {
        case VisualProfile::Broadcast: return "Editorial";
        case VisualProfile::Compact: return "Compacto";
        case VisualProfile::HighContrast: return "Alto contraste";
    }
    return "Editorial";
}

string visualProfileDescription(VisualProfile profile) {
    switch (profile) {
        case VisualProfile::Broadcast:
            return "Mas aire, paneles amplios y una presencia visual tipo portada manager.";
        case VisualProfile::Compact:
            return "Reduce espacios muertos para priorizar densidad de informacion.";
        case VisualProfile::HighContrast:
            return "Refuerza legibilidad y contraste de bordes, acentos y texto clave.";
    }
    return "Mas aire, paneles amplios y una presencia visual tipo portada manager.";
}

string menuMusicModeLabel(MenuMusicMode mode) {
    switch (mode) {
        case MenuMusicMode::Off: return "Silencio";
        case MenuMusicMode::MainMenuOnly: return "Solo portada";
        case MenuMusicMode::FrontendPages: return "Todo el frontend";
    }
    return "Solo portada";
}

string menuMusicModeDescription(MenuMusicMode mode) {
    switch (mode) {
        case MenuMusicMode::Off:
            return "Desactiva la musica del frontend sin tocar el resto de la configuracion.";
        case MenuMusicMode::MainMenuOnly:
            return "Reproduce el tema solo en la portada principal.";
        case MenuMusicMode::FrontendPages:
            return "Extiende el tema a portada, ajustes y creditos del frontend.";
    }
    return "Reproduce el tema solo en la portada principal.";
}

string menuAudioFadeLabel(bool enabled) {
    return enabled ? "Fade activo" : "Cambio directo";
}

string menuAudioFadeDescription(bool enabled) {
    return enabled
        ? "Usa fade-in y fade-out cortos al entrar o salir del frontend."
        : "Activa o corta la musica sin transicion intermedia.";
}

string menuThemeLabel(const GameSettings& settings) {
    if (toLower(trim(settings.menuThemeId)) == "el-crack") return "El Crack";
    if (settings.menuThemeId.empty()) return "Tema por defecto";
    return settings.menuThemeId;
}

string settingsSummary(const GameSettings& settings) {
    return "Volumen " + volumeLabel(settings.volume) +
           " | Dificultad " + difficultyLabel(settings.difficulty) +
           " | Velocidad " + simulationSpeedLabel(settings.simulationSpeed) +
           " | Simulacion " + simulationModeLabel(settings.simulationMode);
}

vector<string> detailedSettingsSummary(const GameSettings& settings) {
    return {
        "Volumen: " + volumeLabel(settings.volume),
        "Dificultad: " + difficultyLabel(settings.difficulty),
        "Velocidad: " + simulationSpeedLabel(settings.simulationSpeed),
        "Simulacion: " + simulationModeLabel(settings.simulationMode),
        "Idioma: " + languageLabel(settings.language),
        "Texto: " + textSpeedLabel(settings.textSpeed),
        "Visual: " + visualProfileLabel(settings.visualProfile),
        "Musica: " + menuMusicModeLabel(settings.menuMusicMode),
        "Audio: " + menuAudioFadeLabel(settings.menuAudioFade),
        "Tema: " + menuThemeLabel(settings)
    };
}

bool isDetailedSimulation(const GameSettings& settings) {
    return settings.simulationMode == SimulationMode::Detailed;
}

bool shouldPlayMenuMusic(const GameSettings& settings, bool onMainMenu, bool onFrontendPage) {
    if (settings.menuMusicMode == MenuMusicMode::Off || settings.volume <= 0) return false;
    if (settings.menuMusicMode == MenuMusicMode::MainMenuOnly) return onMainMenu;
    return onFrontendPage;
}

bool sanitize(GameSettings& settings) {
    const GameSettings original = settings;
    settings.volume = clampVolume(settings.volume);
    if (trim(settings.menuThemeId).empty()) settings.menuThemeId = "el-crack";
    return settings.volume != original.volume ||
           settings.menuThemeId != original.menuThemeId;
}

bool loadFromDisk(GameSettings& settings, const string& path) {
    vector<string> lines;
    const string resolvedPath = normalizeSettingsPath(path.empty() ? settingsFilePath() : path);
    if (!readTextFileLines(resolvedPath, lines)) {
        sanitize(settings);
        return false;
    }

    map<string, string> values;
    for (const string& rawLine : lines) {
        string line = trim(rawLine);
        if (line.empty() || line[0] == '#') continue;
        size_t separator = line.find('=');
        if (separator == string::npos) continue;
        string key = toLower(trim(line.substr(0, separator)));
        string value = trim(line.substr(separator + 1));
        if (!key.empty()) values[key] = value;
    }

    if (values.count("volume")) settings.volume = parseIntValue(values["volume"], settings.volume);
    if (values.count("difficulty")) settings.difficulty = parseDifficulty(values["difficulty"]);
    if (values.count("simulation_speed")) settings.simulationSpeed = parseSimulationSpeed(values["simulation_speed"]);
    if (values.count("simulation_mode")) settings.simulationMode = parseSimulationMode(values["simulation_mode"]);
    if (values.count("language")) settings.language = parseLanguage(values["language"]);
    if (values.count("text_speed")) settings.textSpeed = parseTextSpeed(values["text_speed"]);
    if (values.count("visual_profile")) settings.visualProfile = parseVisualProfile(values["visual_profile"]);
    if (values.count("menu_music_mode")) settings.menuMusicMode = parseMenuMusicMode(values["menu_music_mode"]);
    if (values.count("menu_audio_fade")) settings.menuAudioFade = parseBoolValue(values["menu_audio_fade"], settings.menuAudioFade);
    if (values.count("menu_theme")) settings.menuThemeId = trim(values["menu_theme"]);

    sanitize(settings);
    return true;
}

bool saveToDisk(const GameSettings& settings, const string& path) {
    const string resolvedPath = normalizeSettingsPath(path.empty() ? settingsFilePath() : path);
    const string folder = pathFilename(resolvedPath) == resolvedPath ? string() : resolvedPath.substr(0, resolvedPath.size() - pathFilename(resolvedPath).size());
    if (!folder.empty()) ensureDirectory(folder);

    ofstream file(resolvedPath, ios::binary | ios::trunc);
    if (!file.is_open()) return false;

    GameSettings safe = settings;
    sanitize(safe);
    file << "version=2\n";
    file << "volume=" << safe.volume << "\n";
    file << "difficulty=" << encodeDifficulty(safe.difficulty) << "\n";
    file << "simulation_speed=" << encodeSimulationSpeed(safe.simulationSpeed) << "\n";
    file << "simulation_mode=" << encodeSimulationMode(safe.simulationMode) << "\n";
    file << "language=" << encodeLanguage(safe.language) << "\n";
    file << "text_speed=" << encodeTextSpeed(safe.textSpeed) << "\n";
    file << "visual_profile=" << encodeVisualProfile(safe.visualProfile) << "\n";
    file << "menu_music_mode=" << encodeMenuMusicMode(safe.menuMusicMode) << "\n";
    file << "menu_audio_fade=" << encodeBool(safe.menuAudioFade) << "\n";
    file << "menu_theme=" << (trim(safe.menuThemeId).empty() ? "el-crack" : safe.menuThemeId) << "\n";
    return file.good();
}

int uiPulseDelayMs(const GameSettings& settings) {
    return max(0, baseDelayFromSimulationSpeed(settings.simulationSpeed) + textDelayModifier(settings.textSpeed));
}

int pageTransitionDelayMs(const GameSettings& settings) {
    const int base = baseDelayFromSimulationSpeed(settings.simulationSpeed);
    return max(0, base / 2 + textDelayModifier(settings.textSpeed) / 2);
}

int audioFadeStepDelayMs(const GameSettings& settings) {
    if (!settings.menuAudioFade) return 0;
    const int base = uiPulseDelayMs(settings);
    return clampInt(base / 4, 8, 48);
}

void cycleVolume(GameSettings& settings) {
    settings.volume = nextVolume(settings.volume);
}

void cycleDifficulty(GameSettings& settings) {
    settings.difficulty = nextDifficulty(settings.difficulty);
}

void cycleSimulationSpeed(GameSettings& settings) {
    settings.simulationSpeed = nextSimulationSpeed(settings.simulationSpeed);
}

void cycleSimulationMode(GameSettings& settings) {
    settings.simulationMode = nextSimulationMode(settings.simulationMode);
}

void cycleLanguage(GameSettings& settings) {
    settings.language = nextLanguage(settings.language);
}

void cycleTextSpeed(GameSettings& settings) {
    settings.textSpeed = nextTextSpeed(settings.textSpeed);
}

void cycleVisualProfile(GameSettings& settings) {
    settings.visualProfile = nextVisualProfile(settings.visualProfile);
}

void cycleMenuMusicMode(GameSettings& settings) {
    settings.menuMusicMode = nextMenuMusicMode(settings.menuMusicMode);
}

void toggleMenuAudioFade(GameSettings& settings) {
    settings.menuAudioFade = !settings.menuAudioFade;
}

void applyNewCareerDifficulty(Career& career, const GameSettings& settings) {
    if (!career.myTeam) return;

    Team& team = *career.myTeam;
    if (settings.difficulty == GameDifficulty::Accessible) {
        const long long boost = max(25000LL, team.budget / 20);
        team.budget += boost;
        team.morale = clampInt(team.morale + 4, 0, 100);
        career.boardConfidence = clampInt(career.boardConfidence + 8, 0, 100);
        career.managerReputation = clampInt(career.managerReputation + 4, 1, 100);
        career.addNews("Configuracion de dificultad accesible: el proyecto arranca con algo mas de aire economico y respaldo.");
        return;
    }

    if (settings.difficulty == GameDifficulty::Challenging) {
        const long long cut = max(25000LL, team.budget / 20);
        team.budget = max(0LL, team.budget - cut);
        team.morale = clampInt(team.morale - 4, 0, 100);
        career.boardConfidence = clampInt(career.boardConfidence - 8, 0, 100);
        career.managerReputation = clampInt(career.managerReputation - 4, 1, 100);
        career.addNews("Configuracion desafiante: la directiva exige mas y el arranque economico es mas ajustado.");
    }
}

void applyQuickMatchDifficulty(Team& userTeam, Team& opponentTeam, const GameSettings& settings) {
    if (settings.difficulty == GameDifficulty::Accessible) {
        userTeam.morale = clampInt(userTeam.morale + 6, 0, 100);
        for (auto& player : userTeam.players) {
            player.fitness = clampInt(player.fitness + 4, 1, 100);
            player.currentForm = clampInt(player.currentForm + 3, 1, 100);
        }
        return;
    }

    if (settings.difficulty == GameDifficulty::Challenging) {
        opponentTeam.morale = clampInt(opponentTeam.morale + 6, 0, 100);
        userTeam.morale = clampInt(userTeam.morale - 3, 0, 100);
        for (auto& player : opponentTeam.players) {
            player.fitness = clampInt(player.fitness + 4, 1, 100);
            player.currentForm = clampInt(player.currentForm + 3, 1, 100);
        }
    }
}

}  // namespace game_settings
