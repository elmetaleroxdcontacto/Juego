#include "engine/game_settings.h"

#include "utils/utils.h"

#include <algorithm>
#include <array>

using namespace std;

namespace game_settings {

string gameTitle() {
    return "Chilean Footballito";
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
            return "Prioriza avanzar mas rapido por menus y simulaciones sin tocar la logica base.";
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

string settingsSummary(const GameSettings& settings) {
    return "Volumen " + volumeLabel(settings.volume) +
           " | Dificultad " + difficultyLabel(settings.difficulty) +
           " | Velocidad " + simulationSpeedLabel(settings.simulationSpeed) +
           " | Simulacion " + simulationModeLabel(settings.simulationMode);
}

bool isDetailedSimulation(const GameSettings& settings) {
    return settings.simulationMode == SimulationMode::Detailed;
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
