#include "engine/debt_system.h"
#include <algorithm>
#include <sstream>

DebtStatus calculateDebtStatus(long long currentBudget, long long debt, long long weeklyIncome) {
    DebtStatus status;
    status.totalDebt = debt;
    status.monthlyDebtPayment = debt / 12;  // Pago en 12 cuotas
    
    // Calcular severidad
    if (weeklyIncome > 0) {
        int debtToIncomeRatio = (debt * 100) / (weeklyIncome * 52);
        status.debtSeverity = std::min(100, debtToIncomeRatio);
    } else {
        status.debtSeverity = 100;
    }
    
    // Determinar restricciones
    if (status.debtSeverity < 30) {
        status.canBuyPlayers = true;
        status.canOfferHighSalaries = true;
        status.canMakeLongTermContracts = true;
        status.transferBudgetReduction = 0;
    } else if (status.debtSeverity < 60) {
        status.canBuyPlayers = true;
        status.canOfferHighSalaries = false;
        status.canMakeLongTermContracts = true;
        status.transferBudgetReduction = 20;
    } else if (status.debtSeverity < 85) {
        status.canBuyPlayers = false;
        status.canOfferHighSalaries = false;
        status.canMakeLongTermContracts = false;
        status.transferBudgetReduction = 50;
    } else {
        status.canBuyPlayers = false;
        status.canOfferHighSalaries = false;
        status.canMakeLongTermContracts = false;
        status.transferBudgetReduction = 100;
        status.inDefaultRisk = true;
        status.weeksUntilSeizure = 26;  // 6 meses
    }
    
    return status;
}

void updateDebtStatus(DebtStatus& status, long long weeklyProfit) {
    // Reducer deuda con ganancias
    if (weeklyProfit > 0) {
        status.totalDebt = std::max(0LL, status.totalDebt - weeklyProfit);
    } else {
        // Aumentar deuda si tiene pérdida
        status.totalDebt += -weeklyProfit;
    }
    
    // Recalcular severidad
    if (status.inDefaultRisk && weeklyProfit > 0) {
        status.weeksUntilSeizure--;
        if (status.weeksUntilSeizure <= 0) {
            // Evento: Embargo del club
            status.totalDebt = status.totalDebt * 2;  // Penalización
            status.transferBudgetReduction = 100;
        }
    }
}

std::vector<FinancialSanction> getActiveSanctions(const DebtStatus& status) {
    std::vector<FinancialSanction> sanctions;
    
    if (status.debtSeverity > 70) {
        sanctions.push_back(FinancialSanction::TRANSFER_BAN);
    }
    if (status.debtSeverity > 60) {
        sanctions.push_back(FinancialSanction::WAGE_CAP);
    }
    if (status.debtSeverity > 80) {
        sanctions.push_back(FinancialSanction::YOUTH_ACADEMY_BANNED);
    }
    if (status.inDefaultRisk) {
        sanctions.push_back(FinancialSanction::POINTS_DEDUCTION);
    }
    
    return sanctions;
}

void applyFinancialSanctions(DebtStatus& status) {
    auto sanctions = getActiveSanctions(status);
    
    for (const auto& sanction : sanctions) {
        if (sanction == FinancialSanction::TRANSFER_BAN) {
            status.canBuyPlayers = false;
        } else if (sanction == FinancialSanction::WAGE_CAP) {
            status.canOfferHighSalaries = false;
        } else if (sanction == FinancialSanction::YOUTH_ACADEMY_BANNED) {
            status.canMakeLongTermContracts = false;
        }
    }
}

void removeFinancialSanction(DebtStatus& status, FinancialSanction sanction) {
    if (sanction == FinancialSanction::TRANSFER_BAN && status.debtSeverity <= 70) {
        status.canBuyPlayers = true;
    } else if (sanction == FinancialSanction::WAGE_CAP && status.debtSeverity <= 60) {
        status.canOfferHighSalaries = true;
    }
}

std::string getDebtReport(const DebtStatus& status) {
    std::stringstream ss;
    ss << "\n=== ESTADO FINANCIERO ===\n";
    ss << "Deuda Total: $" << status.totalDebt << "\n";
    ss << "Pago Mensual: $" << status.monthlyDebtPayment << "\n";
    ss << "Severidad de Deuda: " << status.debtSeverity << "/100\n";
    
    if (status.debtSeverity < 30) {
        ss << "Estado: Saludable\n";
    } else if (status.debtSeverity < 60) {
        ss << "Estado: Moderado\n";
    } else if (status.debtSeverity < 85) {
        ss << "Estado: Crítico\n";
    } else {
        ss << "Estado: EN RIESGO DE EMBARGO\n";
        ss << "Semanas hasta embargo: " << status.weeksUntilSeizure << "\n";
    }
    
    ss << "\nRestricciones:\n";
    if (!status.canBuyPlayers) ss << "- NO puedes comprar jugadores\n";
    if (!status.canOfferHighSalaries) ss << "- Salarios limitados\n";
    if (!status.canMakeLongTermContracts) ss << "- No puedes hacer contratos largos\n";
    if (status.transferBudgetReduction > 0) {
        ss << "- Presupuesto reducido " << status.transferBudgetReduction << "%\n";
    }
    
    return ss.str();
}

bool canAffordTransfer(const DebtStatus& status, long long transferCost, long long playerWage) {
    if (!status.canBuyPlayers) return false;
    
    // Verificar si el costo se ajusta al presupuesto reducido
    long long adjustedCost = transferCost * (100 - status.transferBudgetReduction) / 100;
    
    return true;  // Simplificado - requeriría presupuesto actual real
}

bool triggerFinancialCrisis(DebtStatus& status, long long threshold) {
    if (status.totalDebt > threshold) {
        status.inDefaultRisk = true;
        status.weeksUntilSeizure = 26;
        return true;
    }
    return false;
}
