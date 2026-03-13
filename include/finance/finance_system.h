#pragma once

#include "engine/models.h"

#include <string>

struct WeeklyFinanceReport {
    long long wageBill = 0;
    long long sponsorIncome = 0;
    long long matchdayIncome = 0;
    long long merchandisingIncome = 0;
    long long bonusIncome = 0;
    long long debtService = 0;
    long long netCashFlow = 0;
    long long transferBuffer = 0;
    std::string riskLevel;
};

namespace finance_system {

long long calculateWeeklyPayroll(const Team& team);
long long calculateTransferBuffer(const Team& team);
WeeklyFinanceReport projectWeeklyReport(const Team& team, int pointsEarned = 0, bool homeMatch = true);

}  // namespace finance_system
