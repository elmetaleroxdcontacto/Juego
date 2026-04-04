#pragma once

#include <string>
#include <vector>

// Sistema de Estrés del Manager - Afecta decisiones y habilidades

struct ManagerStressState {
    int stressLevel = 50;  // 0-100: nivel de estrés
    int energy = 100;  // 0-100: energía disponible
    int mentalFortitude = 60;  // 0-100: resistencia mental
    int decisionQuality = 100;  // 100-150: modificador de calidad de decisión
    
    // Factores de estrés
    int consecutiveLosses = 0;
    int winStreak = 0;
    int pressureIntensity = 0;  // Según objetivos y expectativas
    int criticismLevel = 0;  // Por prensa y resultados
    
    // Recuperación
    int restDays = 0;
    int lastVictoryWeek = 0;
    int moralVictoriesCount = 0;
};

struct StressEvent {
    std::string type;  // "loss", "win", "criticism", "milestone", "scandal", "injury_star_player"
    int stressImpact = 0;  // +/- estrés
    std::string description;
};

// Modificadores por estrés en decisiones
struct ManagerDecisionModifier {
    int transferAccuracy = 0;  // -penalty si muy estresado
    int tacticalDecision = 0;  // Decisiones tácticas pueden ser erráticas
    int scoutingJudgment = 0;  // Mal scouting si estresado
    int contractNegotiation = 0;  // Puede pagar más/menos de lo debido
};

// Funciones
ManagerStressState initializeManagerStress();
void updateManagerStress(ManagerStressState& state, const StressEvent& event);
ManagerDecisionModifier getDecisionModifier(const ManagerStressState& state);
int applyStressToDecision(int baseDecisionValue, const ManagerStressState& state);
void reduceStressWithRest(ManagerStressState& state, int daysRest);
std::string getManagerStatusReport(const ManagerStressState& state);
void triggerStressBreakdown(ManagerStressState& state);  // Evento crítico si estrés > 90
