#include "engine/manager_stress.h"
#include <algorithm>
#include <sstream>
#include <cmath>

ManagerStressState initializeManagerStress() {
    ManagerStressState state;
    state.stressLevel = 50;
    state.energy = 100;
    state.mentalFortitude = 60;
    state.decisionQuality = 100;
    state.consecutiveLosses = 0;
    state.winStreak = 0;
    state.pressureIntensity = 0;
    state.criticismLevel = 0;
    state.restDays = 0;
    state.lastVictoryWeek = 0;
    state.moralVictoriesCount = 0;
    return state;
}

void updateManagerStress(ManagerStressState& state, const StressEvent& event) {
    // Aplicar impacto del evento
    state.stressLevel += event.stressImpact;
    state.stressLevel = std::max(0, std::min(100, state.stressLevel));
    
    // Actualizar contadores según tipo de evento
    if (event.type == "loss") {
        state.consecutiveLosses++;
        state.winStreak = 0;
        state.criticismLevel = std::min(100, state.criticismLevel + 15);
        state.energy = std::max(0, state.energy - 10);
    } else if (event.type == "win") {
        state.winStreak++;
        state.consecutiveLosses = 0;
        state.stressLevel = std::max(0, state.stressLevel - 10);
        state.criticismLevel = std::max(0, state.criticismLevel - 5);
        state.energy = std::min(100, state.energy + 5);
        state.moralVictoriesCount++;
    } else if (event.type == "criticism") {
        state.criticismLevel = std::min(100, state.criticismLevel + 10);
    } else if (event.type == "injury_star_player") {
        state.stressLevel = std::min(100, state.stressLevel + 20);
        state.energy = std::max(0, state.energy - 15);
    } else if (event.type == "scandal") {
        state.stressLevel = std::min(100, state.stressLevel + 25);
        state.mentalFortitude = std::max(20, state.mentalFortitude - 10);
    }
    
    // Actualizar presión según rachas
    if (state.consecutiveLosses >= 3) {
        state.pressureIntensity = std::min(100, state.pressureIntensity + (state.consecutiveLosses - 2) * 5);
    } else {
        state.pressureIntensity = std::max(0, state.pressureIntensity - 2);
    }
    
    // La presión afecta estrés
    state.stressLevel = std::min(100, state.stressLevel + (state.pressureIntensity / 20));
}

ManagerDecisionModifier getDecisionModifier(const ManagerStressState& state) {
    ManagerDecisionModifier mod;
    
    // Calcular penalties por estrés
    int stressPenalty = (state.stressLevel - 50) / 10;  // -5 a +5 range
    
    mod.transferAccuracy = -stressPenalty * 3;  // Más estrés = peor criterio de fichajes
    mod.tacticalDecision = -stressPenalty * 2;
    mod.scoutingJudgment = -stressPenalty * 4;
    mod.contractNegotiation = -stressPenalty * 5;  // Error en negociaciones = caro
    
    // Energía también afecta
    if (state.energy < 30) {
        mod.transferAccuracy -= 5;
        mod.tacticalDecision -= 3;
        mod.scoutingJudgment -= 7;
    }
    
    return mod;
}

int applyStressToDecision(int baseDecisionValue, const ManagerStressState& state) {
    ManagerDecisionModifier mod = getDecisionModifier(state);
    int modifiedValue = baseDecisionValue;
    
    // Aplicar modificador aleatorio con cierta convergencia al base
    int randomComponent = (rand() % 20) - 10;  // -10 a +10
    if (state.stressLevel > 75) {
        randomComponent = (rand() % 30) - 15;  // Más aleatorio si muy estresado
    }
    
    modifiedValue += randomComponent;
    modifiedValue += (mod.transferAccuracy + mod.tacticalDecision) / 2;
    
    return modifiedValue;
}

void reduceStressWithRest(ManagerStressState& state, int daysRest) {
    state.restDays += daysRest;
    
    // Cada 2 días de descanso reduce estrés en 5
    int recoveryAmount = (state.restDays / 2) * 5;
    state.stressLevel = std::max(0, state.stressLevel - recoveryAmount);
    state.energy = std::min(100, state.energy + (daysRest * 3));
    state.mentalFortitude = std::min(100, state.mentalFortitude + (daysRest * 2));
    
    state.restDays = state.restDays % 2;  // Mantener resto
}

std::string getManagerStatusReport(const ManagerStressState& state) {
    std::stringstream ss;
    ss << "\n=== ESTADO DEL MANAGER ===\n";
    ss << "Nivel de Estrés: " << state.stressLevel << "/100 ";
    
    if (state.stressLevel < 30) ss << "(Relajado)\n";
    else if (state.stressLevel < 50) ss << "(Normal)\n";
    else if (state.stressLevel < 75) ss << "(Tensionado)\n";
    else ss << "(Crítico)\n";
    
    ss << "Energía: " << state.energy << "/100\n";
    ss << "Fortaleza Mental: " << state.mentalFortitude << "/100\n";
    ss << "Calidad de Decisión: " << state.decisionQuality << "%\n";
    
    if (state.consecutiveLosses > 0) {
        ss << "Derrotas Consecutivas: " << state.consecutiveLosses << "\n";
    }
    if (state.winStreak > 0) {
        ss << "Victorias Consecutivas: " << state.winStreak << "\n";
    }
    
    ss << "Presión Mediática: " << state.pressureIntensity << "/100\n";
    ss << "Críticas: " << state.criticismLevel << "/100\n";
    
    return ss.str();
}

void triggerStressBreakdown(ManagerStressState& state) {
    // Si el estrés es > 90, hay riesgo de colapso
    if (state.stressLevel > 90) {
        state.mentalFortitude -= 15;
        state.decisionQuality = 60;  // Decisiones muy pobres
        
        // Posible evento: solicitud de vacaciones, amenaza de renuncia
        // En una versión ui, se mostraría: "El manager demanda tiempo libre inmediato"
        
        if (state.mentalFortitude <= 0) {
            // Colapso total - el manager es reemplazado o toma licencia médica
            // Evento crítico del juego
        }
    }
}
