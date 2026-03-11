#pragma once

#include <cstddef>
#include <string>
#include <vector>

struct DataValidationIssue {
    std::string severity;
    std::string category;
    std::string division;
    std::string team;
    std::string player;
    std::string detail;
    std::string suggestion;
};

struct DataValidationReport {
    bool ok;
    int divisionsScanned;
    int teamsScanned;
    int playersScanned;
    int errorCount;
    int warningCount;
    std::vector<DataValidationIssue> issues;
    std::vector<std::string> lines;
};

struct ValidationSuiteSummary {
    bool ok;
    std::vector<std::string> lines;
};

struct RuntimeValidationSummary {
    bool ok;
    int errorCount;
    int warningCount;
    std::vector<std::string> lines;
};

struct StartupValidationSummary {
    bool ok;
    int errorCount;
    int warningCount;
    std::vector<std::string> lines;
};

DataValidationReport buildRosterDataValidationReport();
bool writeRosterDataValidationReport(const std::string& path);
RuntimeValidationSummary validateLoadedCareerData(const struct Career& career, std::size_t maxLines = 12);
StartupValidationSummary buildStartupValidationSummary(std::size_t maxLines = 8, bool forceRefresh = false);
ValidationSuiteSummary buildValidationSuiteSummary();
int runValidationSuite(bool verbose = true);
