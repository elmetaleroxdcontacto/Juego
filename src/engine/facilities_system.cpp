#include "engine/facilities_system.h"
#include <algorithm>
#include <sstream>

Sponsor createRandomSponsor(int prestige) {
    Sponsor sponsor;
    
    std::vector<std::string> sponsorNames = {
        "Cerveza Corona", "Nike", "Adidas", "Puma", "Banco Santander",
        "Coca-Cola", "Claro", "Movistar", "ENGIE", "BCI"
    };
    sponsor.name = sponsorNames[rand() % sponsorNames.size()];
    
    // Pago basado en prestigio
    sponsor.annualPayment = (prestige * 50000) + (rand() % 2000000);
    
    // Objetivos según prestigio
    if (prestige < 50) {
        sponsor.minimumFinishPosition = 12;
        sponsor.minimumWinsRequired = 5;
    } else if (prestige < 70) {
        sponsor.minimumFinishPosition = 8;
        sponsor.minimumWinsRequired = 10;
    } else {
        sponsor.minimumFinishPosition = 4;
        sponsor.minimumWinsRequired = 15;
    }
    
    sponsor.minimumPointsRequired = sponsor.minimumWinsRequired * 3;
    sponsor.bonusOnCompletion = sponsor.annualPayment / 3;
    sponsor.penaltyOnFailure = -(sponsor.annualPayment / 4);
    
    // Algunos sponsors tienen objetivos específicos
    if (rand() % 2) {
        std::vector<std::string> objectives = {"youth_development", "european_qualification", "win_domestic_cup"};
        sponsor.objectives.push_back(objectives[rand() % objectives.size()]);
    }
    
    return sponsor;
}

long long calculateSponsorIncome(const SponsorshipSystem& system) {
    long long income = 0;
    for (const auto& sponsor : system.activeSponors) {
        income += sponsor.annualPayment;
    }
    return income;
}

bool verifySponsorObjectives(const Sponsor& sponsor, int finish, int wins, int points) {
    // Verificar objetivo de puntos/victorias
    if (points < sponsor.minimumPointsRequired || wins < sponsor.minimumWinsRequired) {
        return false;
    }
    
    // Verificar posición en tabla
    if (finish > sponsor.minimumFinishPosition) {
        return false;
    }
    
    return true;
}

InfrastructureModifiers getModifiersFromFacilities(const FacilityLevel& levels) {
    InfrastructureModifiers mods;
    
    // Cada nivel (1-5) aplica multiplicador
    mods.trainingEffectiveness = 0.8 + (levels.trainingGround * 0.15);  // 0.95 - 1.55
    mods.youthPromotionChance = 0.7 + (levels.youthAcademy * 0.15);    // 0.85 - 1.55
    mods.injuryRecoverySpeed = 0.6 + (levels.medical * 0.20);           // 0.80 - 1.60
    mods.ticketRevenue = 0.5 + (levels.stadium * 0.20);                 // 0.70 - 1.50
    mods.playerHappiness = (levels.facilities - 1) * 3;                 // 0 - 12 bonus happiness
    
    return mods;
}

void upgradeFacility(InfrastructureSystem& system, const std::string& facilityName) {
    int cost = 0;
    
    if (facilityName == "training_ground" && system.levels.trainingGround < 5) {
        system.levels.trainingGround++;
        cost = 500000 * system.levels.trainingGround;
    } else if (facilityName == "youth_academy" && system.levels.youthAcademy < 5) {
        system.levels.youthAcademy++;
        cost = 300000 * system.levels.youthAcademy;
    } else if (facilityName == "medical" && system.levels.medical < 5) {
        system.levels.medical++;
        cost = 400000 * system.levels.medical;
    } else if (facilityName == "stadium" && system.levels.stadium < 5) {
        system.levels.stadium++;
        cost = 1000000 * system.levels.stadium;
    } else if (facilityName == "facilities" && system.levels.facilities < 5) {
        system.levels.facilities++;
        cost = 200000 * system.levels.facilities;
    }
    
    system.totalUpgradeCapacity -= cost;
    system.upgradesThisSeason++;
}

std::string getFacilityReport(const FacilityLevel& levels) {
    std::stringstream ss;
    ss << "\n=== ESTADO DE INSTALACIONES ===\n";
    ss << "Centro de Entrenamiento: " << levels.trainingGround << "/5\n";
    ss << "Academia de Cantera: " << levels.youthAcademy << "/5\n";
    ss << "Departamento Médico: " << levels.medical << "/5\n";
    ss << "Estadio: " << levels.stadium << "/5\n";
    ss << "Comodidades: " << levels.facilities << "/5\n";
    
    InfrastructureModifiers mods = getModifiersFromFacilities(levels);
    ss << "\nModificadores Activos:\n";
    ss << "- Efectividad de Entrenamiento: x" << mods.trainingEffectiveness << "\n";
    ss << "- Probabilidad de Cantera: x" << mods.youthPromotionChance << "\n";
    ss << "- Velocidad Recuperación: x" << mods.injuryRecoverySpeed << "\n";
    ss << "- Ingresos por Taquilla: x" << mods.ticketRevenue << "\n";
    
    return ss.str();
}
