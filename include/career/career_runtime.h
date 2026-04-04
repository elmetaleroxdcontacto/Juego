#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

struct IncomingOfferDecision {
    int action = 3;
    long long counterOffer = 0;
};

enum class WeekSimulationPresentation {
    Compact,
    Detailed
};

using ManagerJobSelectionCallback = int (*)(const Career& career, const std::vector<Team*>& jobs);
using UiMessageCallback = void (*)(const std::string& message);
using IdleCallback = void (*)();
using IncomingOfferDecisionCallback = IncomingOfferDecision (*)(const Career& career,
                                                                const Player& player,
                                                                long long offer,
                                                                long long maxOffer);
using ContractRenewalDecisionCallback = bool (*)(const Career& career,
                                                 const Team& team,
                                                 const Player& player,
                                                 long long demandedWage,
                                                 int demandedWeeks,
                                                 long long demandedClause);

void setManagerJobSelectionCallback(ManagerJobSelectionCallback callback);
void setUiMessageCallback(UiMessageCallback callback);
void setIdleCallback(IdleCallback callback);
void setIncomingOfferDecisionCallback(IncomingOfferDecisionCallback callback);
void setContractRenewalDecisionCallback(ContractRenewalDecisionCallback callback);
void setIdleCallback(IdleCallback callback);
void setWeekSimulationPresentation(WeekSimulationPresentation presentation);

ManagerJobSelectionCallback managerJobSelectionCallback();
UiMessageCallback uiMessageCallback();
IdleCallback idleCallback();
IncomingOfferDecisionCallback incomingOfferDecisionCallback();
ContractRenewalDecisionCallback contractRenewalDecisionCallback();
WeekSimulationPresentation weekSimulationPresentation();

void emitUiMessage(const std::string& message);
