#include "ai/ai_squad_planner.h"

#include "utils/utils.h"

using namespace std;

namespace ai_squad_planner {

int positionNeedScore(const Team& team, const string& pos) {
    int count = 0;
    int skill = 0;
    int oldPlayers = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != pos) continue;
        count++;
        skill += player.skill;
        if (player.age >= 30) oldPlayers++;
    }
    const int avgSkill = count > 0 ? skill / count : 0;
    return 100 - (count * 16 + avgSkill + oldPlayers * 3);
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

    for (const auto& player : team.players) {
        totalAge += player.age;
        totalFitness += player.fitness;
    }
    if (!team.players.empty()) {
        report.averageAge = totalAge / static_cast<int>(team.players.size());
        report.averageFitness = totalFitness / static_cast<int>(team.players.size());
    }

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
    }

    report.priorityPositions.push_back(report.weakestPosition);
    for (const auto& pos : positions) {
        if (pos != report.weakestPosition && positionNeedScore(team, pos) >= report.weakestScore - 6) {
            report.priorityPositions.push_back(pos);
        }
    }
    return report;
}

}  // namespace ai_squad_planner
