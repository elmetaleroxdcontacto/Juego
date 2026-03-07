#include "ai/team_ai.h"

#include "simulation/match_engine_internal.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

int averageAvailableFitness(const Team& team) {
    int total = 0;
    int count = 0;
    for (const auto& player : team.players) {
        if (player.injured || player.matchesSuspended > 0) continue;
        total += player.fitness;
        count++;
    }
    return count > 0 ? total / count : 50;
}

int unavailableCount(const Team& team) {
    int unavailable = 0;
    for (const auto& player : team.players) {
        if (player.injured || player.matchesSuspended > 0) unavailable++;
    }
    return unavailable;
}

bool hasDirectThreat(const Team& team) {
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != "DEL") continue;
        if (playerHasTrait(player, "Competidor") || match_internal::compactToken(player.role) == "poacher" ||
            match_internal::compactToken(player.role) == "objetivo") {
            return true;
        }
    }
    return false;
}

bool applySetting(int& value, int target, int lo, int hi) {
    int clamped = clampInt(target, lo, hi);
    if (value == clamped) return false;
    value = clamped;
    return true;
}

bool applySetting(string& value, const string& target) {
    if (value == target) return false;
    value = target;
    return true;
}

}  // namespace

namespace team_ai {

void adjustCpuTactics(Team& team, const Team& opponent, const Team* myTeam) {
    if (&team == myTeam) return;

    ensureTeamIdentity(team);
    ensureTeamIdentity(const_cast<Team&>(opponent));

    int diff = team.getAverageSkill() - opponent.getAverageSkill();
    int avgFitness = averageAvailableFitness(team);
    int unavailable = unavailableCount(team);
    bool directThreat = hasDirectThreat(team);
    bool chaseBackSpace = opponent.defensiveLine >= 4 && directThreat;
    bool opponentAggressive = opponent.pressingIntensity >= 4 || opponent.tactics == "Pressing";

    if (avgFitness < 66 || unavailable >= 4) {
        team.rotationPolicy = "Rotacion";
    } else if (diff >= 6 && avgFitness >= 72) {
        team.rotationPolicy = "Titulares";
    } else {
        team.rotationPolicy = "Balanceado";
    }

    if (avgFitness < 60 || team.morale <= 35) {
        team.tactics = "Defensive";
    } else if (diff >= 7 && avgFitness >= 70) {
        team.tactics = "Offensive";
    } else if (opponentAggressive && chaseBackSpace) {
        team.tactics = "Counter";
    } else if (team.morale >= 72 && avgFitness >= 72) {
        team.tactics = "Pressing";
    } else {
        team.tactics = "Balanced";
    }

    if (team.tactics == "Offensive") {
        team.pressingIntensity = 4;
        team.defensiveLine = chaseBackSpace ? 3 : 4;
        team.tempo = 4;
        team.width = 4;
        team.markingStyle = "Zonal";
    } else if (team.tactics == "Defensive") {
        team.pressingIntensity = max(1, avgFitness < 58 ? 1 : 2);
        team.defensiveLine = 2;
        team.tempo = 2;
        team.width = 3;
        team.markingStyle = opponent.width >= 4 ? "Zonal" : "Hombre";
    } else if (team.tactics == "Counter") {
        team.pressingIntensity = 3;
        team.defensiveLine = 2;
        team.tempo = 4;
        team.width = 3;
        team.markingStyle = "Zonal";
    } else if (team.tactics == "Pressing") {
        team.pressingIntensity = 4;
        team.defensiveLine = 4;
        team.tempo = 4;
        team.width = 4;
        team.markingStyle = "Zonal";
    } else {
        team.pressingIntensity = 3;
        team.defensiveLine = 3;
        team.tempo = 3;
        team.width = opponent.markingStyle == "Hombre" ? 4 : 3;
        team.markingStyle = avgFitness < 62 ? "Zonal" : "Hombre";
    }

    if (chaseBackSpace) {
        team.matchInstruction = "Juego directo";
    } else if (team.tactics == "Offensive" && opponent.width <= 3) {
        team.matchInstruction = "Por bandas";
    } else if (team.tactics == "Defensive") {
        team.matchInstruction = "Bloque bajo";
    } else if (team.tactics == "Pressing" && avgFitness >= 68) {
        team.matchInstruction = "Contra-presion";
    } else if (avgFitness < 58) {
        team.matchInstruction = "Pausar juego";
    } else {
        team.matchInstruction = "Equilibrado";
    }
}

bool applyInMatchCpuAdjustment(Team& team,
                               const Team& opponent,
                               int minute,
                               int goalsFor,
                               int goalsAgainst,
                               vector<string>* events) {
    bool changed = false;
    int scoreDiff = goalsFor - goalsAgainst;
    int avgFitness = averageAvailableFitness(team);
    string note;

    if (scoreDiff <= -2 && minute >= 55) {
        changed |= applySetting(team.tactics, "Offensive");
        changed |= applySetting(team.matchInstruction, "Presion final");
        changed |= applySetting(team.pressingIntensity, team.pressingIntensity + 1, 1, 5);
        changed |= applySetting(team.defensiveLine, team.defensiveLine + 1, 1, 5);
        changed |= applySetting(team.tempo, team.tempo + 1, 1, 5);
        changed |= applySetting(team.width, 4, 1, 5);
        note = team.name + " cambia a un plan de urgencia para remontar";
    } else if (scoreDiff == -1 && minute >= 65) {
        changed |= applySetting(team.tactics, opponent.tactics == "Defensive" ? "Offensive" : "Pressing");
        changed |= applySetting(team.matchInstruction, opponent.defensiveLine >= 4 ? "Juego directo" : "Por bandas");
        changed |= applySetting(team.pressingIntensity, team.pressingIntensity + 1, 1, 5);
        changed |= applySetting(team.tempo, team.tempo + 1, 1, 5);
        changed |= applySetting(team.defensiveLine, team.defensiveLine + 1, 1, 5);
        note = team.name + " adelanta lineas y asume mas riesgo";
    } else if (scoreDiff >= 1 && minute >= 75) {
        changed |= applySetting(team.tactics, "Defensive");
        changed |= applySetting(team.matchInstruction, avgFitness < 60 ? "Pausar juego" : "Bloque bajo");
        changed |= applySetting(team.pressingIntensity, team.pressingIntensity - 1, 1, 5);
        changed |= applySetting(team.tempo, team.tempo - 1, 1, 5);
        changed |= applySetting(team.defensiveLine, team.defensiveLine - 1, 1, 5);
        note = team.name + " protege la ventaja con un bloque mas conservador";
    } else if (scoreDiff == 0 && minute >= 70 && team.getAverageSkill() >= opponent.getAverageSkill() + 4) {
        changed |= applySetting(team.matchInstruction, "Por bandas");
        changed |= applySetting(team.tempo, team.tempo + 1, 1, 5);
        changed |= applySetting(team.width, 4, 1, 5);
        note = team.name + " busca romper el empate con mas amplitud";
    }

    if (avgFitness < 54 && minute >= 65) {
        bool fatigueChanged = false;
        fatigueChanged |= applySetting(team.pressingIntensity, team.pressingIntensity - 1, 1, 5);
        fatigueChanged |= applySetting(team.tempo, team.tempo - 1, 1, 5);
        if (fatigueChanged) {
            changed = true;
            if (note.empty()) note = team.name + " baja revoluciones por desgaste";
        }
    }

    if (changed && events && !note.empty()) {
        events->push_back(to_string(minute) + "' Ajuste tactico: " + note);
    }
    return changed;
}

}  // namespace team_ai
