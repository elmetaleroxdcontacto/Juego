#include "career/gameplay_reports.h"

#include <sstream>

namespace gameplay_reports {

GameplaySystemsSnapshot captureGameplaySnapshot(const Career& career) {
    GameplaySystemsSnapshot snapshot;
    
    snapshot.dressingRoomReport = generateDressingRoomReport(career.dressingRoomDynamics);
    snapshot.managerStateReport = getManagerStatusReport(career.managerStress);
    
    // Rivalidades - mostrar rivalidades vs próximo rival si es posible
    if (!career.activeTeams.empty() && career.myTeam) {
        snapshot.rivalriesReport = "\n=== RIVALIDADES ===\n";
        snapshot.rivalriesReport += "Tu equipo: " + career.myTeam->name + "\n";
        int rivalCount = 0;
        for (auto team : career.activeTeams) {
            if (team && team != career.myTeam && rivalCount < 3) {
                int intensity = getRivalryIntensity(career.rivalryDynamics, career.myTeam->name, team->name);
                snapshot.rivalriesReport += "- " + team->name + ": intensidad " + std::to_string(intensity) + "/100\n";
                rivalCount++;
            }
        }
    }
    
    snapshot.debtReport = getDebtReport(career.debtStatus);
    snapshot.facilitiesReport = getFacilityReport(career.infrastructure.levels);
    
    return snapshot;
}

std::vector<std::string> getDressingRoomBrief(const DressingRoomDynamics& dynamics) {
    std::vector<std::string> brief;
    
    std::stringstream ss;
    ss << "Vestuario - Moral: " << dynamics.overallCliqueMorale << "/100, "
       << "Tension: " << dynamics.internalTension << "/100, "
       << "Unidad: " << dynamics.unityLevel << "/100";
    brief.push_back(ss.str());
    
    if (dynamics.internalTension > 70) {
        brief.push_back("⚠️ Tension alta en el vestuario - Riesgo de conflictos internos");
    }
    if (dynamics.overallCliqueMorale < 30) {
        brief.push_back("⚠️ Moral muy baja - Puede afectar rendimiento en cancha");
    }
    if (dynamics.unityLevel > 70 && dynamics.overallCliqueMorale > 60) {
        brief.push_back("✓ Vestuario unido - Buen ambiente para competir");
    }
    
    return brief;
}

std::vector<std::string> getManagerCriticalAlerts(const ManagerStressState& state) {
    std::vector<std::string> alerts;
    
    if (state.stressLevel > 85) {
        alerts.push_back("🚨 ESTRÉS CRÍTICO - Pocas deciones de calidad. Requiere descanso urgente.");
    } else if (state.stressLevel > 75) {
        alerts.push_back("⚠️ Estrés muy alto - Decisiones pueden ser erráticas");
    }
    
    if (state.energy < 20) {
        alerts.push_back("⚠️ Energía agotada - Tomar decisiones importantes NO es recomendado");
    }
    
    if (state.consecutiveLosses >= 3) {
        alerts.push_back("⚠️ Racha de " + std::to_string(state.consecutiveLosses) + " derrotas - Presión mediática en aumento");
    }
    
    return alerts;
}

std::vector<std::string> getRivalryHighlights(const RivalryDynamics& dynamics, const std::string& myTeam) {
    std::vector<std::string> highlights;
    
    for (const auto& record : dynamics.rivalries) {
        if ((record.team1 == myTeam || record.team2 == myTeam) && record.intensity > 60) {
            std::string opponent = (record.team1 == myTeam) ? record.team2 : record.team1;
            int winCount = (record.team1 == myTeam) ? record.team1Wins : record.team1Losses;
            int lossCount = (record.team1 == myTeam) ? record.team1Losses : record.team1Wins;
            
            std::stringstream ss;
            ss << "Rivalidad con " << opponent << ": "
               << winCount << "W-" << record.team1Draws << "D-" << lossCount << "L "
               << "(Intensidad: " << record.intensity << "/100)";
            highlights.push_back(ss.str());
        }
    }
    
    return highlights;
}

std::vector<std::string> getDebtAlerts(const DebtStatus& status) {
    std::vector<std::string> alerts;
    
    if (status.debtSeverity > 80) {
        alerts.push_back("🚨 CRISIS FINANCIERA - En riesgo de embargo en " + std::to_string(status.weeksUntilSeizure) + " semanas");
    } else if (status.debtSeverity > 70) {
        alerts.push_back("⚠️ Deuda muy grave - NO puedes realizar fichajes");
    } else if (status.debtSeverity > 50) {
        alerts.push_back("⚠️ Deuda moderada-alta - Presupuesto limitado para transferencias");
    }
    
    if (!status.canOfferHighSalaries && status.debtSeverity > 40) {
        alerts.push_back("⚠️ No puedes ofrecer salarios competitivos");
    }
    
    return alerts;
}

std::vector<std::string> getFacilitySuggestions(const InfrastructureSystem& system, long long budget) {
    std::vector<std::string> suggestions;
    
    // Sugerir mejorar lo que está más bajo y podemos costear
    int minLevel = std::min(std::min(std::min(std::min(system.levels.trainingGround, system.levels.youthAcademy),
                             system.levels.medical), system.levels.stadium), system.levels.facilities);
    
    if (budget > 500000 && minLevel < 5) {
        suggestions.push_back("💡 Tienes presupuesto para mejorar instalaciones");
        
        if (system.levels.trainingGround == minLevel && system.levels.trainingGround < 5) {
            suggestions.push_back("  - Centro de Entrenamiento actual Nivel " + std::to_string(system.levels.trainingGround));
        }
        if (system.levels.medical == minLevel && system.levels.medical < 5) {
            suggestions.push_back("  - Departamento Médico actual Nivel " + std::to_string(system.levels.medical));
        }
        if (system.levels.youthAcademy == minLevel && system.levels.youthAcademy < 5) {
            suggestions.push_back("  - Academia de Cantera actual Nivel " + std::to_string(system.levels.youthAcademy));
        }
    }
    
    return suggestions;
}

}  // namespace gameplay_reports
