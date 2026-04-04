#pragma once

#include <string>
#include <vector>

// Sistema de Deuda y Consecuencias Financieras

struct DebtStatus {
    long long totalDebt = 0;
    long long monthlyDebtPayment = 0;
    int debtSeverity = 0;  // 0-100: qué tan grave es
    bool inDefaultRisk = false;
    int weeksUntilSeizure = -1;  // Semanas hasta embargo (-1 = no en riesgo)
    
    // Restricciones por deuda
    bool canBuyPlayers = true;
    bool canOfferHighSalaries = true;
    bool canMakeLongTermContracts = true;
    int transferBudgetReduction = 0;  // % reducción en presupuesto
};

enum class FinancialSanction {
    NONE = 0,
    TRANSFER_BAN = 1,      // No puede fichar
    WAGE_CAP = 2,          // Limita salarios
    YOUTH_ACADEMY_BANNED = 3,  // No puede usar cantera
    FACILITY_INVESTMENT_BANNED = 4,  // No puede mejorar instalaciones
    POINTS_DEDUCTION = 5   // Descontado de puntos en liga
};

struct FinancialCrisis {
    bool active = false;
    int severity = 0;  // 1-5
    std::vector<FinancialSanction> activeSanctions;
    int weeksRemaining = 0;
    std::string reason;  // e.g., "excessive_debt", "unpaid_salaries", "breach_ffp"
};

// Funciones
DebtStatus calculateDebtStatus(long long currentBudget, long long debt, long long weeklyIncome);
void updateDebtStatus(DebtStatus& status, long long weeklyProfit);
std::vector<FinancialSanction> getActiveSanctions(const DebtStatus& status);
void applyFinancialSanctions(DebtStatus& status);
void removeFinancialSanction(DebtStatus& status, FinancialSanction sanction);
std::string getDebtReport(const DebtStatus& status);
bool canAffordTransfer(const DebtStatus& status, long long transferCost, long long playerWage);
bool triggerFinancialCrisis(DebtStatus& status, long long threshold);
