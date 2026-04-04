#include "utils/career_safety.h"
#include <functional>
#include <algorithm>

namespace career_safety {

Team* getTeamOrNull(Career& career, int index) {
    if (index < 0 || index >= static_cast<int>(career.activeTeams.size())) {
        return nullptr;
    }
    return career.activeTeams[index];
}

const Team* getTeamOrNull(const Career& career, int index) {
    if (index < 0 || index >= static_cast<int>(career.activeTeams.size())) {
        return nullptr;
    }
    return career.activeTeams[index];
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
    for (auto* teamPtr : career.activeTeams) {
        if (teamPtr) {
            callback(*teamPtr);
        }
    }
}

void forEachDivisionTeamConst(const Career& career, 
                             std::function<void(const Team&)> callback) {
    for (const auto* teamPtr : career.activeTeams) {
        if (teamPtr) {
            callback(*teamPtr);
        }
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
