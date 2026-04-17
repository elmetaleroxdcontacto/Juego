#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

namespace weekly_focus_service {

struct WeeklyFocusSnapshot {
    std::string headline;
    std::vector<std::string> priorityLines;
    std::vector<std::string> kpiLines;
    std::vector<std::string> tutorialLines;
};

WeeklyFocusSnapshot buildWeeklyFocusSnapshot(const Career& career,
                                             std::size_t priorityLimit = 3,
                                             std::size_t kpiLimit = 4,
                                             std::size_t tutorialLimit = 3);

std::vector<std::string> buildPriorityLines(const Career& career, std::size_t limit = 3);
std::vector<std::string> buildTutorialLines(const Career& career, std::size_t limit = 3);

}  // namespace weekly_focus_service
