#include "engine/rivalry_system.h"
#include <algorithm>
#include <sstream>

void initializeRivalries(const std::vector<std::string>& teamNames, RivalryDynamics& dynamics) {
    for (size_t i = 0; i < teamNames.size(); i++) {
        for (size_t j = i + 1; j < teamNames.size(); j++) {
            RivalryRecord record;
            record.team1 = teamNames[i];
            record.team2 = teamNames[j];
            record.encounters = 0;
            record.team1Wins = 0;
            record.team1Draws = 0;
            record.team1Losses = 0;
            record.intensity = rand() % 30;  // Rivalidad inicial aleatoria
            
            // Detectar rivalidades locales (simplificado - buscar en names)
            bool sameRegion = (teamNames[i].find("Santiago") != std::string::npos && 
                              teamNames[j].find("Santiago") != std::string::npos);
            record.isLocalRivalry = sameRegion;
            
            // Rivalidades históricas (hardcoded para ligas chilenas)
            if ((teamNames[i] == "Colo-Colo" && teamNames[j] == "Universidad de Chile") ||
                (teamNames[i] == "Universidad de Chile" && teamNames[j] == "Colo-Colo")) {
                record.isHistoricRivalry = true;
                record.intensity = 85;
            }
            
            dynamics.rivalries.push_back(record);
            dynamics.pressLevel[teamNames[i]] = 0;
            dynamics.pressLevel[teamNames[j]] = 0;
        }
    }
}

RivalryRecord* getRivalryRecord(RivalryDynamics& dynamics, const std::string& team1, const std::string& team2) {
    for (auto& record : dynamics.rivalries) {
        if ((record.team1 == team1 && record.team2 == team2) ||
            (record.team1 == team2 && record.team2 == team1)) {
            return &record;
        }
    }
    return nullptr;
}

void updateRivalryRecord(RivalryRecord& record, int team1GoalsFor, int team1GoalsAgainst) {
    record.encounters++;
    
    if (team1GoalsFor > team1GoalsAgainst) {
        record.team1Wins++;
    } else if (team1GoalsFor == team1GoalsAgainst) {
        record.team1Draws++;
    } else {
        record.team1Losses++;
    }
    
    // Aumentar intensidad con each encounter
    record.intensity = std::min(100, record.intensity + 2);
    
    // Si hay racha significativa, intensidad sube más
    int team1WinRate = (record.team1Wins * 100) / std::max(1, record.encounters);
    if (team1WinRate > 70 || team1WinRate < 30) {
        record.intensity = std::min(100, record.intensity + 5);
    }
}

int getRivalryIntensity(const RivalryDynamics& dynamics, const std::string& team1, const std::string& team2) {
    for (const auto& record : dynamics.rivalries) {
        if ((record.team1 == team1 && record.team2 == team2) ||
            (record.team1 == team2 && record.team2 == team1)) {
            return record.intensity;
        }
    }
    return 0;
}

std::string generateRivalryNarrative(const RivalryRecord& record) {
    std::stringstream ss;
    ss << "\n=== HISTORIAL CONTRA " << record.team2 << " ===\n";
    ss << "Encuentros: " << record.encounters << "\n";
    ss << "Victorias: " << record.team1Wins << "\n";
    ss << "Empates: " << record.team1Draws << "\n";
    ss << "Derrotas: " << record.team1Losses << "\n";
    ss << "Intensidad: " << record.intensity << "/100\n";
    
    if (record.isHistoricRivalry) {
        ss << "Esta es una rivalidad HISTÓRICA\n";
    }
    if (record.isLocalRivalry) {
        ss << "Esta es una rivalidad LOCAL\n";
    }
    
    // Generar narrativa según récord
    int winRate = (record.team1Wins * 100) / std::max(1, record.encounters);
    if (winRate > 70) {
        ss << "Dominio claro: Ganamos la mayoría de encuentros.\n";
    } else if (winRate < 30) {
        ss << "Desventaja: Ellos nos ganan regularmente.\n";
    } else {
        ss << "Partidos parejos: Nadie domina la rivalidad.\n";
    }
    
    return ss.str();
}

void submitTacticalChange(RealTimeMatchState& state, const InMatchTacticalChange& change) {
    if (state.tacticalChangesUsed < state.maxTacticalChanges) {
        state.submittedChanges.push_back(change);
        state.tacticalChangesUsed++;
        state.canMakeTacticalChange = (state.tacticalChangesUsed < state.maxTacticalChanges);
    }
}

void applyTacticalChanges(std::string& tactics, std::string& formation, const RealTimeMatchState& state) {
    // Aplicar cambios que sean válidos para el minuto actual
    for (const auto& change : state.submittedChanges) {
        if (change.minute <= state.currentMinute) {
            tactics = change.newTactics;
            if (!change.newFormation.empty()) {
                formation = change.newFormation;
            }
        }
    }
}

bool canMakeTacticalChange(const RealTimeMatchState& state, int currentMinute) {
    // Restricciones de cuándo se puede hacer cambio
    if (state.tacticalChangesUsed >= state.maxTacticalChanges) return false;
    if (currentMinute < 10) return false;  // No cambios en primeros 10 minutos
    if (currentMinute > 85) return false;  // No cambios en últimos 5 minutos
    
    // Verificar spacing - no dos cambios en menos de 15 minutos
    if (!state.submittedChanges.empty()) {
        int lastChangeMinute = state.submittedChanges.back().minute;
        if (currentMinute - lastChangeMinute < 15) {
            return false;
        }
    }
    
    return true;
}

int tacticChangeEnergyCost(const std::string& change_type) {
    if (change_type == "formation_change") {
        return 20;  // Alto costo
    } else if (change_type == "intensity_boost") {
        return 15;
    } else if (change_type == "defensive_focus") {
        return 10;
    } else {
        return 15;
    }
}
