#pragma once

#include "engine/models.h"
#include "engine/social_system.h"
#include "engine/manager_stress.h"
#include "engine/rivalry_system.h"
#include "engine/debt_system.h"
#include "engine/facilities_system.h"

#include <string>
#include <vector>

// Genera reportes visuales de los nuevos sistemas de gameplay
namespace gameplay_reports {

struct GameplaySystemsSnapshot {
    std::string dressingRoomReport;
    std::string managerStateReport;
    std::string rivalriesReport;
    std::string debtReport;
    std::string facilitiesReport;
};

// Recopila todos los reportes de los sistemas de gameplay
GameplaySystemsSnapshot captureGameplaySnapshot(const Career& career);

// Obtiene un reporte corto de la dinámicas de vestuario para la UI
std::vector<std::string> getDressingRoomBrief(const DressingRoomDynamics& dynamics);

// Obtiene advertencias si el estrés del manager es crítico
std::vector<std::string> getManagerCriticalAlerts(const ManagerStressState& state);

// Obtiene palabras clave sobre las rivalidades
std::vector<std::string> getRivalryHighlights(const RivalryDynamics& dynamics, const std::string& myTeam);

// Obtiene advertencias sobre la deuda
std::vector<std::string> getDebtAlerts(const DebtStatus& status);

// Obtiene sugerencias de mejora de infraestructura
std::vector<std::string> getFacilitySuggestions(const InfrastructureSystem& system, long long budget);

}  // namespace gameplay_reports
