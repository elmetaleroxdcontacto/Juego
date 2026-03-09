#include "finance/finance_system.h"

#include "competition/competition.h"

using namespace std;

namespace finance_system {

long long calculateWeeklyPayroll(const Team& team) {
    long long wages = 0;
    for (const auto& player : team.players) wages += player.wage;
    return wages * getCompetitionConfig(team.division).wageFactor / 100;
}

long long calculateTransferBuffer(const Team& team) {
    long long buffer = team.budget - calculateWeeklyPayroll(team) * 8 - team.debt / 16;
    if (team.debt > team.sponsorWeekly * 18) buffer -= team.debt / 24;
    return max(0LL, buffer);
}

WeeklyFinanceReport projectWeeklyReport(const Team& team, int pointsEarned, bool homeMatch) {
    WeeklyFinanceReport report;
    report.wageBill = calculateWeeklyPayroll(team);
    report.sponsorIncome = team.sponsorWeekly;
    report.matchdayIncome = homeMatch ? static_cast<long long>(team.fanBase) * 1800LL * team.stadiumLevel : 0LL;
    report.debtService = max(0LL, team.debt / 180);
    report.netCashFlow = report.sponsorIncome + report.matchdayIncome + static_cast<long long>(pointsEarned) * 5000LL -
                         report.wageBill - report.debtService;
    report.transferBuffer = calculateTransferBuffer(team);
    report.riskLevel = report.netCashFlow >= 0 && report.transferBuffer >= report.wageBill * 4 ? "Controlado"
                      : report.transferBuffer >= report.wageBill * 2                      ? "Vigilado"
                                                                                           : "Alto";
    return report;
}

}  // namespace finance_system
