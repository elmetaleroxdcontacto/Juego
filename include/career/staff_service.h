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

std::vector<StaffProfile> buildStaffProfiles(const Team& team);
std::string weakestStaffRole(const Team& team);
std::string buildStaffDigest(const Team& team, std::size_t limit = 7);

}  // namespace staff_service
