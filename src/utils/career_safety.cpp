#include "utils/career_safety.h"
#include <functional>
#include <algorithm>

namespace career_safety {

Team* getTeamOrNull(Career& career, int index) {
    return career.getActiveTeamAt(index);
}

const Team* getTeamOrNull(const Career& career, int index) {
    return career.getActiveTeamAt(index);
}

Player* findPlayerInTeam(Team& team, const std::string& playerName) {
    auto it = std::find_if(team.players.begin(), team.players.end(),
                          [&playerName](const Player& p) { return p.name == playerName; });
    if (it != team.players.end()) {
        return &(*it);
    }
    return nullptr;
}

const Player* findPlayerInTeam(const Team& team, const std::string& playerName) {
    auto it = std::find_if(team.players.begin(), team.players.end(),
                          [&playerName](const Player& p) { return p.name == playerName; });
    if (it != team.players.end()) {
        return &(*it);
    }
    return nullptr;
}

void forEachDivisionTeam(Career& career, 
                        std::function<void(Team&)> callback) {
    for (int i = 0; i < career.getActiveTeamCount(); ++i) {
        if (Team* team = career.getActiveTeamAt(i)) callback(*team);
    }
}

void forEachDivisionTeamConst(const Career& career, 
                             std::function<void(const Team&)> callback) {
    for (int i = 0; i < career.getActiveTeamCount(); ++i) {
        if (const Team* team = career.getActiveTeamAt(i)) callback(*team);
    }
}

bool getMatchAtWeekDay(const Career& career, int weekIdx, int dayIdx, 
                      int& outHomeIdx, int& outAwayIdx) {
    if (weekIdx < 0 || weekIdx >= static_cast<int>(career.schedule.size())) {
        return false;
    }
    const auto& weekMatches = career.schedule[weekIdx];
    if (dayIdx < 0 || dayIdx >= static_cast<int>(weekMatches.size())) {
        return false;
    }
    outHomeIdx = weekMatches[dayIdx].first;
    outAwayIdx = weekMatches[dayIdx].second;
    return true;
}

}  // namespace career_safety
