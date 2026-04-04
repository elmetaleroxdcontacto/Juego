#include "engine/rival_ai.h"
#include <algorithm>
#include <sstream>
#include <cmath>

RivalAI createRivalAI(const std::string& teamName, int prestigeLevel) {
    RivalAI ai;
    ai.personality.teamName = teamName;
    
    // Asignar estilo basado en prestigio
    if (prestigeLevel < 40) {
        ai.personality.playstyle = "defensive";
        ai.personality.adaptability = 40;
        ai.personality.tactical_awareness = 35;
    } else if (prestigeLevel < 60) {
        ai.personality.playstyle = "balanced";
        ai.personality.adaptability = 55;
        ai.personality.tactical_awareness = 50;
    } else if (prestigeLevel < 80) {
        ai.personality.playstyle = "aggressive";
        ai.personality.adaptability = 65;
        ai.personality.tactical_awareness = 65;
    } else {
        ai.personality.playstyle = "possession";
        ai.personality.adaptability = 75;
        ai.personality.tactical_awareness = 80;
    }
    
    // Impredibilidad - equipos peores cometen más errores
    ai.personality.unpredictability = 100 - prestigeLevel;
    
    return ai;
}

std::string RivalAI::decideTactics(const std::string& playerTeamFormation, const std::string& playerTactics) const {
    std::string decidedTactics = personality.playstyle;
    
    // Buscar memorias contra este oponente
    if (!memoryBank.empty()) {
        auto lastMemory = memoryBank.back();
        
        // Si ganó recientemente, mantiene táctica
        if (lastMemory.lastMatchOutcome == 1 && lastMemory.consecutiveVsThisTeam > 0) {
            return lastMemory.favoredTactics.empty() ? personality.playstyle : lastMemory.favoredTactics.front();
        }
        
        // Si perdió, intenta cambiar táctica (adaptación)
        if (lastMemory.lastMatchOutcome == -1 && personality.adaptability > 60) {
            // Cambiar a táctica contraria
            if (playerTactics == "aggressive") {
                decidedTactics = "defensive";
            } else if (playerTactics == "defensive") {
                decidedTactics = "counter-attack";
            } else {
                decidedTactics = "attacking";
            }
        }
    }
    
    return decidedTactics;
}

std::vector<TacticalAdjustment> RivalAI::generateInMatchAdjustments(
    int currentMinute, int myScore, int oppScore, 
    const std::string& myTactics, const std::string& oppTactics) const {
    
    std::vector<TacticalAdjustment> adjustments;
    
    // Lógica de reacción simple del rival
    if (myScore > oppScore) {
        // Si va ganando, presiona más (si es agresivo)
        if (personality.playstyle == "aggressive") {
            TacticalAdjustment adj;
            adj.adjustmentType = "intensity_boost";
            adj.intensity = 15;
            adj.minuteToApply = currentMinute + 5;
            adj.reactive = true;
            adjustments.push_back(adj);
        } else if (personality.playstyle == "defensive") {
            // Si va ganando y es defensivo, se cierra más
            TacticalAdjustment adj;
            adj.adjustmentType = "defensive_focus";
            adj.intensity = 10;
            adj.minuteToApply = currentMinute + 3;
            adj.reactive = true;
            adjustments.push_back(adj);
        }
    } else if (oppScore > myScore && personality.adaptability > 50) {
        // Si va perdiendo y es adaptable, intenta atacar más
        TacticalAdjustment adj;
        adj.adjustmentType = "formation_change";
        adj.intensity = 20;
        adj.minuteToApply = currentMinute + 10;
        adj.reactive = true;
        adjustments.push_back(adj);
    }
    
    // Ajustes en minutos críticos (45', 80'+)
    if (currentMinute == 45 || currentMinute == 80) {
        if (myScore == oppScore && personality.playstyle != "balanced") {
            TacticalAdjustment adj;
            adj.adjustmentType = "intensity_boost";
            adj.intensity = 10;
            adj.minuteToApply = currentMinute;
            adj.reactive = false;
            adjustments.push_back(adj);
        }
    }
    
    return adjustments;
}

void RivalAI::analyzeOpponentPattern(const std::string& opponentTeam, int goalsScoredByOpp, int goalsConceded) {
    // Buscar o crear memoria
    auto memIt = std::find_if(memoryBank.begin(), memoryBank.end(),
        [&opponentTeam](const RivalMemory& m) { return m.opponentName == opponentTeam; });
    
    RivalMemory* memory;
    if (memIt != memoryBank.end()) {
        memory = &(*memIt);
    } else {
        memoryBank.push_back(RivalMemory{});
        memory = &memoryBank.back();
        memory->opponentName = opponentTeam;
    }
    
    memory->matchesPlayed++;
    if (goalsScoredByOpp > goalsConceded) {
        memory->wins++;
    } else if (goalsScoredByOpp == goalsConceded) {
        memory->draws++;
    } else {
        memory->losses++;
    }
    
    // Identificar patrones
    if (goalsScoredByOpp > 2) {
        memory->identifiedWeaknesses.push_back("defensive_fragility");
    }
    if (goalsConceded < 1) {
        memory->identifiedStrengths.push_back("strong_defense");
    }
}

int RivalAI::getErrorProbability(int minuteOfMatch) {
    // Probabilidad base según unpredictability
    int baseErrorChance = personality.unpredictability / 2;
    
    // Aumenta con fatiga (minutos finales)
    if (minuteOfMatch > 75) {
        baseErrorChance += 10;
    }
    
    // Disminuye si está ganando
    if (minuteOfMatch < 45) {
        baseErrorChance -= 5;
    }
    
    return std::max(0, std::min(100, baseErrorChance));
}

void applyRivalAIAdjustments(std::string& rivalTactics, const std::vector<TacticalAdjustment>& adjustments) {
    for (const auto& adj : adjustments) {
        if (adj.adjustmentType == "defensive_focus") {
            rivalTactics = "defensive";
        } else if (adj.adjustmentType == "intensity_boost") {
            if (rivalTactics != "aggressive") {
                rivalTactics = "aggressive";
            }
        } else if (adj.adjustmentType == "counter_setup") {
            rivalTactics = "counter-attack";
        }
        // formation_change se manejaría en otro lado
    }
}
