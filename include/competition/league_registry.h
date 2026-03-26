#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

bool reloadLeagueRegistry();
const std::vector<DivisionInfo>& listRegisteredDivisions();
const std::vector<std::string>& leagueRegistryWarnings();
std::string registeredCompetitionRulesPath();
std::string canonicalDivisionId(const std::string& raw);
bool isRegisteredDivisionId(const std::string& raw);
std::string divisionDisplayName(const std::string& id);
