#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Sistema de Rivalidades Dinámicas entre Equipos

struct RivalryRecord {
    std::string team1;
    std::string team2;
    int encounters = 0;
    int team1Wins = 0;
    int team1Draws = 0;
    int team1Losses = 0;
    int lastMeetingWeek = 0;
    int intensity = 0;  // 0-100: qué tan intensa es la rivalidad
    bool isLocalRivalry = false;  // Mismo estado/ciudad
    bool isHistoricRivalry = false;  // Tradicional
};

struct RivalryDynamics {
    std::vector<RivalryRecord> rivalries;
    std::unordered_map<std::string, int> pressLevel;  // Presión mediática por rival
};

// Sistema de Cambios Tácticos en Tiempo Real

struct InMatchTacticalChange {
    int minute = 0;
    std::string newTactics;  // Nueva táctica a aplicar
    std::string newFormation;  // Nueva formación (opcional)
    std::string reason;  // e.g., "trailing_score", "key_injury", "player_needs_rest"
    int moralImpact = 0;  // Impacto en moral de cambio
};

struct RealTimeMatchState {
    int currentMinute = 0;
    int myScore = 0;
    int oppScore = 0;
    std::vector<InMatchTacticalChange> submittedChanges;
    bool canMakeTacticalChange = true;  // Limitado por manager energy/stress
    int tacticalChangesUsed = 0;
    int maxTacticalChanges = 3;  // Por partido
};

// Funciones de Rivalidades
void initializeRivalries(const std::vector<std::string>& teamNames, RivalryDynamics& dynamics);
RivalryRecord* getRivalryRecord(RivalryDynamics& dynamics, const std::string& team1, const std::string& team2);
void updateRivalryRecord(RivalryRecord& record, int team1GoalsFor, int team1GoalsAgainst);
int getRivalryIntensity(const RivalryDynamics& dynamics, const std::string& team1, const std::string& team2);
std::string generateRivalryNarrative(const RivalryRecord& record);

// Funciones de Cambios Tácticos en Tiempo Real
void submitTacticalChange(RealTimeMatchState& state, const InMatchTacticalChange& change);
void applyTacticalChanges(std::string& tactics, std::string& formation, const RealTimeMatchState& state);
bool canMakeTacticalChange(const RealTimeMatchState& state, int currentMinute);
int tacticChangeEnergyCost(const std::string& change_type);
