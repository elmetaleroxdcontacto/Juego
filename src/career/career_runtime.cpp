#include "career/career_runtime.h"

#include <iostream>

using namespace std;

namespace {

ManagerJobSelectionCallback g_managerJobSelectionCallback = nullptr;
UiMessageCallback g_uiMessageCallback = nullptr;
IdleCallback g_idleCallback = nullptr;
IncomingOfferDecisionCallback g_incomingOfferDecisionCallback = nullptr;
ContractRenewalDecisionCallback g_contractRenewalDecisionCallback = nullptr;
WeekSimulationPresentation g_weekSimulationPresentation = WeekSimulationPresentation::Detailed;

}  // namespace

void setManagerJobSelectionCallback(ManagerJobSelectionCallback callback) {
    g_managerJobSelectionCallback = callback;
}

void setUiMessageCallback(UiMessageCallback callback) {
    g_uiMessageCallback = callback;
}

void setIdleCallback(IdleCallback callback) {
    g_idleCallback = callback;
}

void setIncomingOfferDecisionCallback(IncomingOfferDecisionCallback callback) {
    g_incomingOfferDecisionCallback = callback;
}

void setContractRenewalDecisionCallback(ContractRenewalDecisionCallback callback) {
    g_contractRenewalDecisionCallback = callback;
}

void setWeekSimulationPresentation(WeekSimulationPresentation presentation) {
    g_weekSimulationPresentation = presentation;
}

ManagerJobSelectionCallback managerJobSelectionCallback() {
    return g_managerJobSelectionCallback;
}

UiMessageCallback uiMessageCallback() {
    return g_uiMessageCallback;
}

IncomingOfferDecisionCallback incomingOfferDecisionCallback() {
    return g_incomingOfferDecisionCallback;
}

ContractRenewalDecisionCallback contractRenewalDecisionCallback() {
    return g_contractRenewalDecisionCallback;
}

IdleCallback idleCallback() {
    return g_idleCallback;
}

WeekSimulationPresentation weekSimulationPresentation() {
    return g_weekSimulationPresentation;
}

void emitUiMessage(const string& message) {
    if (g_uiMessageCallback) {
        g_uiMessageCallback(message);
    } else {
        cout << message << endl;
    }
}
