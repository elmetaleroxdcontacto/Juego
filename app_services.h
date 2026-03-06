#pragma once

#include "models.h"
#include "validators.h"

#include <string>
#include <vector>

struct ServiceResult {
    bool ok = false;
    std::vector<std::string> messages;
};

ServiceResult startCareerService(Career& career,
                                 const std::string& divisionId,
                                 const std::string& teamName,
                                 const std::string& managerName);
ServiceResult loadCareerService(Career& career);
ServiceResult saveCareerService(Career& career);
ServiceResult simulateCareerWeekService(Career& career);
ValidationSuiteSummary runValidationService();
