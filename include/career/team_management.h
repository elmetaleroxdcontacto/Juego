#pragma once

#include "engine/models.h"

#include <string>

namespace team_mgmt {

int playerIndexByName(const Team& team, const std::string& name);
void detachPlayerFromSelections(Team& team, const std::string& playerName);
void applyDepartureShock(Team& team, const Player& player);

}  // namespace team_mgmt
