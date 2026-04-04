#pragma once

#include "engine/models.h"
#include <vector>
#include <string>
#include <functional>

// === CAREER SAFETY UTILITIES ===
// Phase 3 Refactoring: Provide safe abstractions for common Career operations
// These functions reduce raw pointer usage and provide bounds checking

namespace career_safety {

// Safe team access with logging
Team* getTeamOrNull(Career& career, int index);
const Team* getTeamOrNull(const Career& career, int index);

// Safe player lookup within a team
Player* findPlayerInTeam(Team& team, const std::string& playerName);
const Player* findPlayerInTeam(const Team& team, const std::string& playerName);

// Division team iteration (avoids direct activeTeams access)
void forEachDivisionTeam(Career& career, 
                        std::function<void(Team&)> callback);
void forEachDivisionTeamConst(const Career& career, 
                             std::function<void(const Team&)> callback);

// Safe match lookup in schedule
bool getMatchAtWeekDay(const Career& career, int weekIdx, int dayIdx, 
                      int& outHomeIdx, int& outAwayIdx);

}  // namespace career_safety
