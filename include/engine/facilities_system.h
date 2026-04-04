#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Sistema de Patrocinios Dinámicos

struct Sponsor {
    std::string name;
    long long annualPayment = 0;
    int minimumFinishPosition = 0;  // e.g., top 4, top 8
    int minimumWinsRequired = 0;
    int minimumPointsRequired = 0;
    
    // Objetivos específicos
    std::vector<std::string> objectives;  // e.g., "win_cup", "european_qualification", "youth_development"
    
    // Conflictos potenciales
    std::vector<std::string> rivalTeams;  // Sponsors no quieren jugar contra estos
    bool completed = false;
    bool failed = false;
    int bonusOnCompletion = 0;
    int penaltyOnFailure = 0;
};

struct SponsorshipSystem {
    std::vector<Sponsor> activeSponors;
    long long totalSponsorshipIncome = 0;
    int prestigeFromSponsorship = 0;
};

// Sistema de Infraestructura con Impacto Real

struct FacilityLevel {
    int trainingGround = 1;  // 1-5: mejor entrenamiento
    int youthAcademy = 1;  // 1-5: mejor cantera
    int medical = 1;  // 1-5: mejor recuperación de lesiones
    int stadium = 1;  // 1-5: más ingresos por taquilla, ambiente local
    int facilities = 1;  // 1-5: afecta felicidad de jugadores
};

struct InfrastructureSystem {
    FacilityLevel levels;
    std::unordered_map<std::string, int> improvementCosts;  // Costo de mejorar cada facility
    int totalUpgradeCapacity = 0;  // Presupuesto disponible para mejoras
    int upgradesThisSeason = 0;
};

// Modificadores aplicados por nivel de infraestructura
struct InfrastructureModifiers {
    float trainingEffectiveness = 1.0;  // Muliplica beneficio de entrenamientos
    float youthPromotionChance = 1.0;  // Probabilidad de talento joven
    float injuryRecoverySpeed = 1.0;  // Semanas menos de lesión
    float ticketRevenue = 1.0;  // Ingresos por asistencia
    float playerHappiness = 0;  // +points directo a felicidad
};

// Funciones
Sponsor createRandomSponsor(int prestige);
long long calculateSponsorIncome(const SponsorshipSystem& system);
bool verifySponsorObjectives(const Sponsor& sponsor, int finish, int wins, int points);
InfrastructureModifiers getModifiersFromFacilities(const FacilityLevel& levels);
void upgradeFacility(InfrastructureSystem& system, const std::string& facilityName);
std::string getFacilityReport(const FacilityLevel& levels);
