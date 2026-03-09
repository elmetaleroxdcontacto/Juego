#include "career/career_runtime.h"

#include <iostream>

using namespace std;

namespace {

ManagerJobSelectionCallback g_managerJobSelectionCallback = nullptr;
UiMessageCallback g_uiMessageCallback = nullptr;
IncomingOfferDecisionCallback g_incomingOfferDecisionCallback = nullptr;
ContractRenewalDecisionCallback g_contractRenewalDecisionCallback = nullptr;

}  // namespace

void setManagerJobSelectionCallback(ManagerJobSelectionCallback callback) {
    g_managerJobSelectionCallback = callback;
}

void setUiMessageCallback(UiMessageCallback callback) {
    g_uiMessageCallback = callback;
}

void setIncomingOfferDecisionCallback(IncomingOfferDecisionCallback callback) {
    g_incomingOfferDecisionCallback = callback;
}

void setContractRenewalDecisionCallback(ContractRenewalDecisionCallback callback) {
    g_contractRenewalDecisionCallback = callback;
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

void emitUiMessage(const string& message) {
    if (g_uiMessageCallback) {
        g_uiMessageCallback(message);
    } else {
        cout << message << endl;
    }
}
