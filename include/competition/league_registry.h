#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

bool reloadLeagueRegistry();
const std::vector<DivisionInfo>& listRegisteredDivisions();
const std::vector<std::string>& leagueRegistryWarnings();
std::string registeredCompetitionRulesPath();
std::string divisionDisplayName(const std::string& id);
