#pragma once

#include "career/career_runtime.h"
#include "engine/models.h"

#include <string>
#include <vector>

struct WeekSimulationResult {
    bool ok = false;
    int weekBefore = 0;
    int weekAfter = 0;
    int seasonBefore = 0;
    int seasonAfter = 0;
    int boardConfidenceBefore = 0;
    int boardConfidenceAfter = 0;
    bool seasonTransitionTriggered = false;
    std::vector<std::string> messages;
    std::string lastMatchAnalysis;
};

struct SeasonStepResult {
    bool ok = false;
    WeekSimulationResult week;
};

class SeasonService {
public:
    SeasonStepResult simulateWeek(Career& career,
                                  IncomingOfferDecisionCallback offerDecision = nullptr,
                                  ContractRenewalDecisionCallback renewDecision = nullptr,
                                  ManagerJobSelectionCallback managerDecision = nullptr) const;
};
