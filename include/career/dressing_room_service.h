#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

struct DressingRoomSnapshot {
    std::string climate;
    std::string summary;
    int leaderCount = 0;
    int conflictCount = 0;
    int promiseRiskCount = 0;
    int positionPromiseRiskCount = 0;
    int lowMoraleCount = 0;
    int fatigueRiskCount = 0;
    int wantsOutCount = 0;
    std::vector<std::string> groups;
    std::vector<std::string> alerts;
};

namespace dressing_room_service {

DressingRoomSnapshot buildSnapshot(const Team& team, int currentWeek);
DressingRoomSnapshot applyWeeklyUpdate(Career& career, int pointsDelta);
std::string formatSnapshot(const Career& career, std::size_t maxAlerts = 5);

}  // namespace dressing_room_service
