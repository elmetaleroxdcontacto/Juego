#include "career/season_service.h"

#include "career/career_runtime.h"
#include "career/week_simulation.h"

#include <iostream>
#include <sstream>
#include <streambuf>
#include <utility>
#include <vector>

using namespace std;

namespace {

vector<string>* g_seasonMessages = nullptr;

void collectSeasonMessage(const string& message) {
    if (g_seasonMessages && !message.empty()) g_seasonMessages->push_back(message);
}

vector<string> splitOutputLines(const string& text) {
    vector<string> lines;
    istringstream stream(text);
    string line;
    while (getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

class StdoutCapture {
public:
    StdoutCapture() : original_(cout.rdbuf(buffer_.rdbuf())) {}

    ~StdoutCapture() {
        cout.rdbuf(original_);
    }

    string str() const {
        return buffer_.str();
    }

private:
    streambuf* original_;
    ostringstream buffer_;
};

class RuntimeCallbackScope {
public:
    RuntimeCallbackScope(UiMessageCallback uiCallback,
                         IncomingOfferDecisionCallback offerCallback,
                         ContractRenewalDecisionCallback renewCallback,
                         ManagerJobSelectionCallback managerCallback)
        : previousUi_(uiMessageCallback()),
          previousOffer_(incomingOfferDecisionCallback()),
          previousRenew_(contractRenewalDecisionCallback()),
          previousManager_(managerJobSelectionCallback()) {
        setUiMessageCallback(uiCallback);
        setIncomingOfferDecisionCallback(offerCallback);
        setContractRenewalDecisionCallback(renewCallback);
        setManagerJobSelectionCallback(managerCallback);
    }

    ~RuntimeCallbackScope() {
        setManagerJobSelectionCallback(previousManager_);
        setContractRenewalDecisionCallback(previousRenew_);
        setIncomingOfferDecisionCallback(previousOffer_);
        setUiMessageCallback(previousUi_);
    }

private:
    UiMessageCallback previousUi_;
    IncomingOfferDecisionCallback previousOffer_;
    ContractRenewalDecisionCallback previousRenew_;
    ManagerJobSelectionCallback previousManager_;
};

}  // namespace

SeasonStepResult SeasonService::simulateWeek(Career& career,
                                             IncomingOfferDecisionCallback offerDecision,
                                             ContractRenewalDecisionCallback renewDecision,
                                             ManagerJobSelectionCallback managerDecision) const {
    SeasonStepResult result;
    WeekSimulationResult week;
    week.weekBefore = career.currentWeek;
    week.seasonBefore = career.currentSeason;
    week.boardConfidenceBefore = career.boardConfidence;

    vector<string> messages;
    g_seasonMessages = &messages;
    StdoutCapture stdoutCapture;
    {
        RuntimeCallbackScope callbackScope(collectSeasonMessage, offerDecision, renewDecision, managerDecision);
        simulateCareerWeek(career);
    }
    g_seasonMessages = nullptr;

    week.ok = true;
    week.weekAfter = career.currentWeek;
    week.seasonAfter = career.currentSeason;
    week.boardConfidenceAfter = career.boardConfidence;
    week.seasonTransitionTriggered =
        week.seasonAfter != week.seasonBefore || (week.weekBefore > 0 && week.weekAfter < week.weekBefore);
    week.lastMatchAnalysis = career.lastMatchAnalysis;
    vector<string> legacyLines = splitOutputLines(stdoutCapture.str());
    messages.insert(messages.end(), legacyLines.begin(), legacyLines.end());
    week.messages = std::move(messages);
    if (week.messages.empty()) week.messages.push_back("Semana simulada.");

    result.ok = true;
    result.week = std::move(week);
    return result;
}
