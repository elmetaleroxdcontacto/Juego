#include "ai/ai_squad_planner.h"

#include "utils/utils.h"

#include <algorithm>
#include <map>

using namespace std;

namespace {

bool isHighUpsideYouth(const Player& player) {
    return player.age <= 21 && player.potential >= player.skill + 8;
}

int saleCandidateScore(const SquadNeedReport& report,
                       const map<string, int>& positionCounts,
                       const Player& player) {
    const string position = normalizePosition(player.position);
    int score = 0;
    const bool unusedSenior = player.age >= 24 && player.startsThisSeason == 0 && player.matchesPlayed <= 1;

    if (player.wantsToLeave) score += 28;
    if (player.happiness < 46) score += 12;
    if (unusedSenior) score += 18;
    if (player.age >= 30) score += 10;
    if (player.contractWeeks <= 20 && player.age >= 24) score += 8;
    if (position == report.surplusPosition) score += 10;
    if (player.potential <= player.skill + 2) score += 6;
    if (player.fitness < 55) score += 4;
    if (player.promisedRole == "Titular" && player.startsThisSeason >= 2) score -= 14;
    if (player.promisedRole == "Proyecto") score -= 10;
    if (isHighUpsideYouth(player)) score -= 18;
    if (position == "ARQ" && positionCounts.count(position) && positionCounts.at(position) <= 2) score -= 25;
    return score;
}

}  // namespace

namespace ai_squad_planner {

int positionNeedScore(const Team& team, const string& pos) {
    int count = 0;
    int skill = 0;
    int oldPlayers = 0;
    int lowFitness = 0;
    int youthUpside = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != pos) continue;
        count++;
        skill += player.skill;
        if (player.age >= 30) oldPlayers++;
        if (player.fitness < 64) lowFitness++;
        if (player.age <= 21) youthUpside += max(0, player.potential - player.skill);
    }
    const int avgSkill = count > 0 ? skill / count : 0;
    return 110 - (count * 18 + avgSkill + oldPlayers * 3 + lowFitness * 3 + youthUpside / 4);
}

SquadNeedReport analyzeSquad(const Team& team) {
    static const vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    SquadNeedReport report;
    report.weakestPosition = "MED";
    report.surplusPosition = "MED";
    report.weakestScore = -100000;
    report.surplusScore = -100000;
    int totalAge = 0;
    int totalFitness = 0;
    int rotationRisk = 0;
    map<string, int> positionCounts;

    for (const auto& player : team.players) {
        totalAge += player.age;
        totalFitness += player.fitness;
        if (player.fitness < 60 || player.matchesSuspended > 0 || player.injured) rotationRisk++;
        positionCounts[normalizePosition(player.position)]++;
        if (player.age >= 24 && player.startsThisSeason == 0 && player.matchesPlayed <= 1) {
            report.unusedSeniorPlayers++;
        }
    }
    if (!team.players.empty()) {
        report.averageAge = totalAge / static_cast<int>(team.players.size());
        report.averageFitness = totalFitness / static_cast<int>(team.players.size());
    }
    report.rotationRisk = rotationRisk;

    for (const auto& pos : positions) {
        const int need = positionNeedScore(team, pos);
        if (need > report.weakestScore) {
            report.weakestScore = need;
            report.weakestPosition = pos;
        }
        const int surplus = -need;
        if (surplus > report.surplusScore) {
            report.surplusScore = surplus;
            report.surplusPosition = pos;
        }

        int count = 0;
        bool youthCover = false;
        for (const auto& player : team.players) {
            if (normalizePosition(player.position) != pos) continue;
            count++;
            if (player.age <= 20 && player.potential >= player.skill + 8) youthCover = true;
        }
        if (count <= (pos == "ARQ" ? 1 : 2)) report.thinPositions.push_back(pos);
        if (youthCover) report.youthCoverPositions.push_back(pos);
    }

    report.priorityPositions = positions;
    sort(report.priorityPositions.begin(), report.priorityPositions.end(), [&](const string& left, const string& right) {
        const int leftNeed = positionNeedScore(team, left);
        const int rightNeed = positionNeedScore(team, right);
        if (leftNeed != rightNeed) return leftNeed > rightNeed;
        return left < right;
    });

    if (!report.priorityPositions.empty()) report.weakestPosition = report.priorityPositions.front();
    for (const auto& pos : positions) {
        if (pos != report.weakestPosition && positionNeedScore(team, pos) >= report.weakestScore - 6) {
            if (find(report.priorityPositions.begin(), report.priorityPositions.end(), pos) == report.priorityPositions.end()) {
                report.priorityPositions.push_back(pos);
            }
        }
    }

    vector<pair<int, string> > saleRanks;
    for (const auto& player : team.players) {
        const int score = saleCandidateScore(report, positionCounts, player);
        if (score >= 18) {
            saleRanks.push_back({score, player.name});
        }
    }
    sort(saleRanks.begin(), saleRanks.end(), [](const auto& left, const auto& right) {
        if (left.first != right.first) return left.first > right.first;
        return left.second < right.second;
    });
    for (size_t i = 0; i < saleRanks.size() && i < 5; ++i) {
        report.saleCandidates.push_back(saleRanks[i].second);
    }
    report.salePressure = static_cast<int>(saleRanks.size());
    if (team.players.size() > 24) report.salePressure += static_cast<int>(team.players.size() - 24);
    if (report.unusedSeniorPlayers >= 3) report.salePressure++;
    return report;
}

}  // namespace ai_squad_planner
