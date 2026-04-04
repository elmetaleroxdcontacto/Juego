#pragma once

#include <vector>
#include <string>

struct Career;
struct Team;

namespace economy_fairplay {

// Fair Play thresholds based on division
struct FairPlayRules {
    std::string division;
    long long maxSalaryCap;        // Maximum total salary
    long long minBudgetReserve;    // Minimum budget reserve
    float maxSalaryPercentage;     // Max salary/revenue ratio
    int maxForeignPlayers;         // Maximum foreign players allowed
    float minYouthPercentage;       // Minimum youth player percentage
};

// Fair Play violation tracking
struct FairPlayViolation {
    std::string type;              // "salary_cap", "budget", "foreign_players", "youth_quota"
    std::string description;
    int severityLevel;             // 1-3 (warning, fine, sanctions)
    long long penaltyAmount;       // Financial penalty if applicable
    bool isActive;
};

class EconomyFairPlaySystem {
public:
    static void initialize(Career& career);
    
    // Check compliance
    static bool checkSalaryCompliance(const Team& team, Career& career);
    static bool checkBudgetCompliance(const Team& team);
    static bool checkForeignPlayerCompliance(const Team& team);
    static bool checkYouthQuotaCompliance(const Team& team);
    
    // Get Fair Play rules for division
    static FairPlayRules getRulesForDivision(const std::string& division);
    
    // Get current violations
    static std::vector<FairPlayViolation> getTeamViolations(const Team& team, Career& career);
    
    // Apply penalties
    static void applyFairPlayPenalties(Team& team, const std::vector<FairPlayViolation>& violations);
    
    // Calculate maximum salary allowed
    static long long getMaxAllowedSalary(const Team& team, Career& career);
    
    // Weekly compliance check
    static void weeklyComplianceCheck(Team& team, Career& career);
    
private:
    static FairPlayRules g_rules[3];  // Primera, Primera B, Segunda
    static bool g_initialized;
};

}  // namespace economy_fairplay
