#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

namespace medical_service {

struct MedicalStatus {
    std::string playerName;
    std::string diagnosis;
    int weeksOut = 0;
    int workloadRisk = 0;
    int relapseRisk = 0;
    std::string recommendation;
    bool unavailable = false;
};

std::vector<MedicalStatus> buildMedicalStatuses(const Team& team);
std::string buildMedicalDigest(const Team& team, std::size_t limit = 6);

}  // namespace medical_service
