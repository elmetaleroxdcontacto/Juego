#pragma once

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

DataValidationReport buildRosterDataValidationReport();
bool writeRosterDataValidationReport(const std::string& path);
ValidationSuiteSummary buildValidationSuiteSummary();
int runValidationSuite(bool verbose = true);
