#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

struct ReportFact {
    std::string label;
    std::string value;
};

struct ReportBlock {
    std::string title;
    std::vector<std::string> lines;
};

struct CareerReport {
    std::string title;
    std::vector<ReportFact> facts;
    std::vector<ReportBlock> blocks;
};

std::string formatMoneyValue(long long value);
std::string detectScoutingNeed(const Team& team);
int competitionGroupForTeam(const Career& career, const Team* team);
LeagueTable buildCompetitionGroupTable(const Career& career, bool north);
LeagueTable buildRelevantCompetitionTable(const Career& career);

CareerReport buildCompetitionReport(const Career& career);
CareerReport buildBoardReport(const Career& career);
CareerReport buildClubReport(const Career& career);
CareerReport buildScoutingReport(const Career& career);
CareerReport buildMatchCenterReport(const Career& career);
CareerReport buildWeeklyDashboardReport(const Career& career);

std::vector<std::string> formatCareerReportLines(const CareerReport& report);
std::string formatCareerReport(const CareerReport& report);
