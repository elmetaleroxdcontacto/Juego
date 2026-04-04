#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Sistema de IA Rival Adaptativa - Personalidad, Memoria, Ajustes Tácticos

struct RivalAIPersonality {
    std::string teamName;
    std::string playstyle;  // "aggressive", "defensive", "balanced", "counter-attack", "possession"
    int adaptability = 50;  // 0-100: qué tan rápido se adapta
    int tactical_awareness = 50;  // 0-100: qué tan buena es su IA táctica
    int unpredictability = 30;  // 0-100: qué tan erradas pueden ser sus decisiones (simula errores)
};

struct RivalMemory {
    std::string opponentName;
    int matchesPlayed = 0;
    int wins = 0;
    int draws = 0;
    int losses = 0;
    
    // Análisis de patrones del rival
    std::vector<std::string> favoredFormations;
    std::vector<std::string> favoredTactics;
    std::string commonPlayPattern;  // e.g., "right-flank-heavy", "long-balls", "pressing"
    
    // Vulnerabilidades identificadas
    std::vector<std::string> identifiedWeaknesses;
    std::vector<std::string> identifiedStrengths;
    
    int lastMatchOutcome = 0;  // -1 loss, 0 draw, 1 win
    int consecutiveVsThisTeam = 0;  // Racha vs este rival
};

struct TacticalAdjustment {
    std::string adjustmentType;  // "formation_change", "intensity_boost", "defensive_focus", "counter_setup"
    int intensity = 0;  // Magnitud del cambio
    int minuteToApply = 0;  // En qué minuto se aplica (-1 = desde inicio)
    bool reactive = false;  // True si es reacción al partido, false si es pre-partido
};

// Función principal de IA rival
class RivalAI {
public:
    RivalAIPersonality personality;
    std::vector<RivalMemory> memoryBank;
    
    // Decide la formación/táctica inicial para contra el jugador
    std::string decideTactics(const std::string& playerTeamFormation, const std::string& playerTactics) const;
    
    // Genera cambios tácticos reactivos durante el partido
    std::vector<TacticalAdjustment> generateInMatchAdjustments(
        int currentMinute, int myScore, int oppScore, 
        const std::string& myTactics, const std::string& oppTactics
    ) const;
    
    // Analiza memoria y ajusta predicciones
    void analyzeOpponentPattern(const std::string& opponentTeam, int goalsScoredByOpp, int goalsConceded);
    
    // Probabilidad de cometer error por estrés/presión
    int getErrorProbability(int minuteOfMatch);
};

// Crea IA rival con personalidad basada en características del equipo
RivalAI createRivalAI(const std::string& teamName, int prestigeLevel);

// Aplica ajustes tácticos durante partido
void applyRivalAIAdjustments(std::string& rivalTactics, const std::vector<TacticalAdjustment>& adjustments);
