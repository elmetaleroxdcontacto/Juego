#include "engine/social_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

DressingRoomDynamics initializeDressingRoom(const std::vector<std::string>& playerNames) {
    DressingRoomDynamics dynamics;
    dynamics.overallCliqueMorale = 50;
    dynamics.internalTension = 0;
    dynamics.unityLevel = 50;
    
    // Crear cliques iniciales basados en características
    // Nota: En una versión completa, leerías atributos de Player
    int numCliques = std::max(2, (int)playerNames.size() / 6);
    
    for (int i = 0; i < numCliques; i++) {
        SocialGroup group;
        group.id = "clique_" + std::to_string(i);
        group.cohesion = 50 + (rand() % 30);
        
        std::vector<std::string> personalities = {"leadership", "troublemakers", "flexible", "professionals"};
        group.personality = personalities[rand() % personalities.size()];
        
        // Asignar miembros a cliques
        const size_t membersInGroup = playerNames.size() / static_cast<size_t>(numCliques);
        const size_t groupOffset = static_cast<size_t>(i) * membersInGroup;
        for (size_t j = 0; j < membersInGroup && groupOffset + j < playerNames.size(); j++) {
            group.memberNames.push_back(playerNames[groupOffset + j]);
        }
        
        // Generar nombre automáticamente
        if (group.personality == "leadership") {
            group.name = "Los Líderes";
        } else if (group.personality == "troublemakers") {
            group.name = "Los Revoltosos";
        } else if (group.personality == "professionals") {
            group.name = "Los Profesionales";
        } else {
            group.name = "El Grupo " + std::to_string(i);
        }
        
        dynamics.cliques.push_back(group);
    }
    
    // Crear relaciones entre algunos jugadores
    for (size_t i = 0; i < playerNames.size() && i < 15; i++) {
        for (size_t j = i + 1; j < playerNames.size() && j < i + 3; j++) {
            PlayerRelationship rel;
            rel.player1 = playerNames[i];
            rel.player2 = playerNames[j];
            rel.chemistry = 40 + (rand() % 50);
            rel.friendship = 40 + (rand() % 50);
            rel.hasConflict = (rand() % 100) < 10;  // 10% de probabilidad inicial
            
            dynamics.relationships.push_back(rel);
        }
    }
    
    return dynamics;
}

void updateCliqueDynamics(DressingRoomDynamics& dynamics, bool hadWin, bool keyMatch) {
    // Después de un ganador, mejorar cohesión
    if (hadWin) {
        dynamics.overallCliqueMorale = std::min(100, dynamics.overallCliqueMorale + 5);
        dynamics.unityLevel = std::min(100, dynamics.unityLevel + 3);
        
        for (auto& clique : dynamics.cliques) {
            clique.cohesion = std::min(100, clique.cohesion + 4);
            if (clique.personality == "leadership") {
                clique.moralBoost = 8;
            } else if (clique.personality == "professionals") {
                clique.moralBoost = 5;
            }
        }
    } else {
        // Después de una derrota, reduce moral y aumenta tensión
        dynamics.overallCliqueMorale = std::max(0, dynamics.overallCliqueMorale - 8);
        dynamics.internalTension = std::min(100, dynamics.internalTension + 5);
        dynamics.unityLevel = std::max(0, dynamics.unityLevel - 3);
        
        for (auto& clique : dynamics.cliques) {
            clique.cohesion = std::max(0, clique.cohesion - 5);
            if (clique.personality == "troublemakers") {
                clique.moralBoost = -10;  // Empeoran en derrotas
                dynamics.internalTension += 3;
            }
        }
    }
    
    if (keyMatch) {
        if (hadWin) {
            dynamics.unityLevel = std::min(100, dynamics.unityLevel + 2);
        } else {
            dynamics.internalTension = std::min(100, dynamics.internalTension + 2);
        }
    }

    // Resolver conflictos cuando la tensión es muy baja
    if (dynamics.internalTension > 70) {
        for (auto& rel : dynamics.relationships) {
            if (!rel.hasConflict && rel.friendship < 40 && (rand() % 100) < 15) {
                rel.hasConflict = true;
                rel.conflictWeeks = 2 + (rand() % 3);
            }
        }
    } else if (dynamics.internalTension < 30) {
        for (auto& rel : dynamics.relationships) {
            if (rel.hasConflict) {
                rel.conflictWeeks--;
                if (rel.conflictWeeks <= 0) {
                    rel.hasConflict = false;
                }
            }
        }
    }
    
    dynamics.weeksTracked++;
}

int getPlayerCliqueMoralBenefit(const DressingRoomDynamics& dynamics, const std::string& playerName) {
    int benefit = 0;
    
    // Buscar el clique del jugador
    for (const auto& clique : dynamics.cliques) {
        bool isInClique = false;
        for (const auto& member : clique.memberNames) {
            if (member == playerName) {
                isInClique = true;
                break;
            }
        }
        
        if (isInClique) {
            // Beneficio basado en cohesión del clique
            benefit += (clique.cohesion - 50) / 5;  // -10 a +10
            benefit += clique.moralBoost / 2;
            break;
        }
    }
    
    // Penalty por conflictos con otros jugadores
    for (const auto& rel : dynamics.relationships) {
        if ((rel.player1 == playerName || rel.player2 == playerName) && rel.hasConflict) {
            benefit -= 5;
        }
    }
    
    return benefit;
}

int applyChemistry(int basePerformance, const DressingRoomDynamics& dynamics, const std::vector<int>& xi) {
    int performanceModifier = 0;
    
    // Calcular chemistry del XI basado en relaciones en el campo
    int strongChemistryCount = 0;
    int conflictCount = 0;
    
    for (size_t i = 0; i < xi.size() && i < 11; i++) {
        for (size_t j = i + 1; j < xi.size() && j < 11; j++) {
            for (const auto& rel : dynamics.relationships) {
                // Simplificado - en versión real necesitaría nombres de jugadores
                if (rel.chemistry > 70) {
                    strongChemistryCount++;
                } else if (rel.hasConflict) {
                    conflictCount++;
                }
            }
        }
    }
    
    performanceModifier += strongChemistryCount * 2;
    performanceModifier -= conflictCount * 3;
    
    return basePerformance + performanceModifier;
}

void resolveDressingRoomConflict(DressingRoomDynamics& dynamics, const std::string& player1, const std::string& player2) {
    for (auto& rel : dynamics.relationships) {
        if ((rel.player1 == player1 && rel.player2 == player2) ||
            (rel.player1 == player2 && rel.player2 == player1)) {
            rel.hasConflict = false;
            rel.conflictWeeks = 0;
            rel.friendship = std::min(100, rel.friendship + 10);
            dynamics.internalTension = std::max(0, dynamics.internalTension - 10);
            break;
        }
    }
}

std::string generateDressingRoomReport(const DressingRoomDynamics& dynamics) {
    std::stringstream ss;
    ss << "\n=== REPORTE DE VESTUARIO ===\n";
    ss << "Moral General: " << dynamics.overallCliqueMorale << "/100\n";
    ss << "Tension Interna: " << dynamics.internalTension << "/100\n";
    ss << "Unidad: " << dynamics.unityLevel << "/100\n";
    ss << "\nCliques:\n";
    
    for (const auto& clique : dynamics.cliques) {
        ss << "- " << clique.name << " (" << clique.personality << "): "
           << clique.cohesion << "/100 cohesion\n";
    }
    
    int conflictCount = 0;
    for (const auto& rel : dynamics.relationships) {
        if (rel.hasConflict) {
            conflictCount++;
        }
    }
    ss << "\nConflictos Activos: " << conflictCount << "\n";
    
    return ss.str();
}
