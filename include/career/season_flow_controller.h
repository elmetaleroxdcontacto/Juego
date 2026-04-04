#pragma once

#include "career/season_service.h"

class SeasonFlowController {
public:
    explicit SeasonFlowController(Career& career);

    SeasonStepResult simulateWeek(IncomingOfferDecisionCallback offerDecision = nullptr,
                                  ContractRenewalDecisionCallback renewDecision = nullptr,
                                  ManagerJobSelectionCallback managerDecision = nullptr,
                                  IdleCallback idleCallback = nullptr);

private:
    Career& career_;
    SeasonService service_;
};
