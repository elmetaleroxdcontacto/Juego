#include "career/season_flow_controller.h"

SeasonFlowController::SeasonFlowController(Career& career) : career_(career) {}

SeasonStepResult SeasonFlowController::simulateWeek(IncomingOfferDecisionCallback offerDecision,
                                                    ContractRenewalDecisionCallback renewDecision,
                                                    ManagerJobSelectionCallback managerDecision) {
    return service_.simulateWeek(career_, offerDecision, renewDecision, managerDecision);
}
