#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Sistema de Cliques y Dinámicas Sociales entre Jugadores

struct SocialGroup {
    std::string id;
    std::string name;  // e.g., "Los Latinos", "Los Veteranos", "La Brigada Joven"
    std::vector<std::string> memberNames;
    int cohesion = 50;  // 0-100: qué tan unidos están
    int moralBoost = 0;  // +moral si cohesion alta, -moral si hay conflictos
    std::string personality;  // "leadership", "troublemakers", "flexible", "professionals"
};

struct PlayerRelationship {
    std::string player1;
    std::string player2;
    int chemistry = 50;  // 0-100: mejor ensamble en campo
    int friendship = 50;  // 0-100: buena onda personal
    bool hasConflict = false;
    int conflictWeeks = 0;
};

struct DressingRoomDynamics {
    int overallCliqueMorale = 50;  // 0-100: ambiente general
    int internalTension = 0;  // 0-100: conflictos internos
    int unityLevel = 50;  // 0-100: cuán unido está el vestuario
    std::vector<SocialGroup> cliques;
    std::vector<PlayerRelationship> relationships;
    int weeksTracked = 0;
};

// Funciones de Sistema Social
DressingRoomDynamics initializeDressingRoom(const std::vector<std::string>& playerNames);
void updateCliqueDynamics(DressingRoomDynamics& dynamics, bool hadWin, bool keyMatch);
int getPlayerCliqueMoralBenefit(const DressingRoomDynamics& dynamics, const std::string& playerName);
int applyChemistry(int basePerformance, const DressingRoomDynamics& dynamics, const std::vector<int>& xi);
void resolveDressingRoomConflict(DressingRoomDynamics& dynamics, const std::string& player1, const std::string& player2);
std::string generateDressingRoomReport(const DressingRoomDynamics& dynamics);
