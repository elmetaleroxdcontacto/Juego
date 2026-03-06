#pragma once

#include <string>
#include <vector>

struct ValidationSuiteSummary {
    bool ok;
    std::vector<std::string> lines;
};

ValidationSuiteSummary buildValidationSuiteSummary();
int runValidationSuite(bool verbose = true);
