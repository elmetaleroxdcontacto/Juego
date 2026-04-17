#pragma once

#include <cstddef>
#include <string>
#include <vector>

class Team;

struct TeamPersonalityProfile {
    std::string coachLabel;
    std::string clubLabel;
    std::string marketLabel;
    std::string youthLabel;
    int pressBias = 50;
    int transitionBias = 50;
    int widthBias = 50;
    int blockBias = 50;
    int controlBias = 50;
    int riskAppetite = 50;
    int adaptability = 50;
    int youthTrust = 50;
    int marketPatience = 50;
    int starterBias = 50;
    int saleBias = 50;
};

TeamPersonalityProfile buildTeamPersonalityProfile(const Team& team);
std::string teamPersonalityHeadline(const Team& team);
std::vector<std::string> teamPersonalitySummaryLines(const Team& team, std::size_t limit = 3);
