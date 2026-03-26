#include "career/transfer_briefing.h"

#include "ai/ai_transfer_manager.h"
#include "career/career_reports.h"
#include "career/manager_advice.h"
#include "utils/utils.h"

#include <algorithm>
#include <utility>

using namespace std;

namespace {

void pushUniqueLine(vector<string>& lines, const string& line) {
    if (line.empty()) return;
    if (find(lines.begin(), lines.end(), line) == lines.end()) {
        lines.push_back(line);
    }
}

string normalizeFilter(const string& filterPos) {
    const string normalized = normalizePosition(filterPos);
    return normalized == "N/A" ? string() : normalized;
}

string joinOrDash(const vector<string>& values) {
    return values.empty() ? string("-") : joinStringValues(values, ", ");
}

}  // namespace

namespace transfer_briefing {

vector<TransferOptionBrief> buildTransferOptions(const Career& career,
                                                 const string& filterPos,
                                                 bool includeShortSquads,
                                                 size_t limit) {
    vector<TransferOptionBrief> options;
    if (!career.myTeam) return options;

    const string normalizedFilter = normalizeFilter(filterPos);
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, *career.myTeam);

    for (const auto& seller : career.allTeams) {
        const Team* sellerPtr = &seller;
        if (sellerPtr == career.myTeam) continue;
        if (!includeShortSquads && seller.players.size() <= 18) continue;

        for (const auto& player : seller.players) {
            if (player.onLoan) continue;
            if (!normalizedFilter.empty() && positionFitScore(player, normalizedFilter) < 70) continue;
            if (player.age > 35) continue;

            const TransferTarget target =
                ai_transfer_manager::evaluateTarget(career, *career.myTeam, seller, player, strategy);

            TransferOptionBrief option;
            option.sellerName = seller.name;
            option.playerName = player.name;
            option.position = player.position;
            option.skill = player.skill;
            option.potential = player.potential;
            option.age = player.age;
            option.contractWeeks = player.contractWeeks;
            option.marketValue = player.value;
            option.releaseClause = player.releaseClause;
            option.wage = player.wage;
            option.availableForLoan = target.availableForLoan;
            option.contractRunningOut = target.contractRunningOut;
            option.onShortlist = target.onShortlist;
            option.scoutingConfidence = target.scoutingConfidence;
            option.readinessScore = target.readinessScore;
            option.medicalRisk = target.medicalRisk;
            option.competitionLabel = manager_advice::buildTransferCompetitionLabel(career, seller, player, target);
            option.actionLabel = manager_advice::buildTransferActionLabel(career, seller, player, target);
            option.packageLabel = manager_advice::buildTransferPackageLabel(target);
            option.scoutingNote = target.scoutingNote;
            option.totalScore = target.totalScore;
            options.push_back(std::move(option));
        }
    }

    sort(options.begin(), options.end(), [](const TransferOptionBrief& left, const TransferOptionBrief& right) {
        if (left.totalScore != right.totalScore) return left.totalScore > right.totalScore;
        if (left.skill != right.skill) return left.skill > right.skill;
        if (left.marketValue != right.marketValue) return left.marketValue < right.marketValue;
        return left.playerName < right.playerName;
    });
    if (options.size() > limit) options.resize(limit);
    return options;
}

vector<TransferOptionBrief> buildPreContractOptions(const Career& career,
                                                    const string& filterPos,
                                                    size_t limit) {
    vector<TransferOptionBrief> filtered;
    const size_t scanLimit = max(limit * 4, static_cast<size_t>(25));
    for (const auto& option : buildTransferOptions(career, filterPos, true, scanLimit)) {
        if (option.contractWeeks > 12) continue;
        filtered.push_back(option);
        if (filtered.size() >= limit) break;
    }
    return filtered;
}

vector<TransferOptionBrief> buildLoanOptions(const Career& career,
                                             const string& filterPos,
                                             size_t limit) {
    vector<TransferOptionBrief> filtered;
    const size_t scanLimit = max(limit * 4, static_cast<size_t>(25));
    for (const auto& option : buildTransferOptions(career, filterPos, false, scanLimit)) {
        if (!option.availableForLoan || option.contractWeeks <= 12) continue;
        filtered.push_back(option);
        if (filtered.size() >= limit) break;
    }
    return filtered;
}

vector<string> buildMarketPulseLines(const Career& career, size_t limit) {
    vector<string> lines;
    if (!career.myTeam) return lines;

    const Team& team = *career.myTeam;
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, team);

    pushUniqueLine(lines,
                   "Necesidad principal: " + (strategy.weakestPosition.empty() ? string("MED") : strategy.weakestPosition) +
                       " | prioridades " + joinOrDash(strategy.priorityPositions));

    if (strategy.needsLiquidity || team.budget < max(180000LL, team.sponsorWeekly * 3)) {
        pushUniqueLine(lines,
                       "Caja ajustada: conviene vender o negociar cesiones antes de cargar salarios altos.");
    } else if (strategy.maxTransferBudget >= max(250000LL, team.sponsorWeekly * 8)) {
        pushUniqueLine(lines,
                       "Ventana abierta: hay margen para atacar un titular si mejora de verdad la posicion prioritaria.");
    } else {
        pushUniqueLine(lines,
                       "Mercado selectivo: mejor una operacion puntual bien scouteada que varias apuestas medias.");
    }

    if (strategy.salePressure >= 3 || !strategy.saleCandidates.empty()) {
        pushUniqueLine(lines,
                       "Presion de salida: el plantel ya sugiere mover " + joinOrDash(strategy.saleCandidates) + ".");
    }

    if (strategy.trustYouthCover && !strategy.youthCoverPositions.empty()) {
        pushUniqueLine(lines,
                       "La cantera cubre " + joinOrDash(strategy.youthCoverPositions) +
                           ", asi que puedes concentrar gasto fuera de esas zonas.");
    }

    if (!career.scoutingShortlist.empty()) {
        pushUniqueLine(lines,
                       "Shortlist activa: " + to_string(career.scoutingShortlist.size()) +
                           " objetivo(s) ya seguidos; actualizar informes puede bajar riesgo antes de ofertar.");
    }

    if (lines.empty()) {
        pushUniqueLine(lines, "Mercado sin tension especial: toca revisar oportunidades, no reaccionar por impulso.");
    }
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}

vector<string> buildTransferOpportunityLines(const Career& career,
                                             const string& filterPos,
                                             size_t limit) {
    vector<string> lines;
    for (const auto& option : buildTransferOptions(career, filterPos, true, limit)) {
        string line = option.playerName + " (" + option.sellerName + ")" +
                      " | " + option.position +
                      " | " + option.actionLabel +
                      " | " + option.competitionLabel +
                      " | " + option.packageLabel +
                      " | listo " + to_string(option.readinessScore) +
                      " | riesgo " + to_string(option.medicalRisk);
        if (option.onShortlist) line += " | shortlist";
        if (option.contractRunningOut) line += " | contrato corto";
        if (option.availableForLoan) line += " | cesion viable";
        lines.push_back(line);
    }
    return lines;
}

}  // namespace transfer_briefing
