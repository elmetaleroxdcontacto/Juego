#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

namespace staff_service {

struct StaffProfile {
    std::string role;
    std::string name;
    int rating = 0;
    long long weeklyCost = 0;
    std::string impact;
    std::string status;
};

struct StaffRecommendation {
    std::string staffRole;
    std::string severity;
    int urgency = 0;
    std::string summary;
    std::string suggestedAction;
};

std::vector<StaffProfile> buildStaffProfiles(const Team& team);
std::string weakestStaffRole(const Team& team);
std::string buildStaffDigest(const Team& team, std::size_t limit = 7);
std::vector<StaffRecommendation> buildStaffRecommendations(const Career& career, std::size_t limit = 6);
std::vector<std::string> buildWeeklyStaffBriefingLines(const Career& career, std::size_t limit = 6);
std::string buildStaffRecommendationDigest(const Career& career, std::size_t limit = 5);

}  // namespace staff_service
