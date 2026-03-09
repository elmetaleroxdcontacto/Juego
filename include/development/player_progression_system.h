#pragma once

#include "engine/models.h"

#include <vector>

namespace development {

void applyMatchExperience(Team& team, const std::vector<int>& participants, std::vector<std::string>* events = nullptr);

}  // namespace development
