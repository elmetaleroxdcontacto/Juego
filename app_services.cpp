#include "app_services.h"

#include "ui.h"

#include <iostream>
#include <sstream>
#include <streambuf>
#include <utility>

using namespace std;

namespace {

struct StdoutCapture {
    streambuf* original = nullptr;
    ostringstream buffer;

    StdoutCapture() : original(cout.rdbuf(buffer.rdbuf())) {}
    ~StdoutCapture() {
        cout.rdbuf(original);
    }

    string str() const {
        return buffer.str();
    }
};

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

vector<string>* g_collectedMessages = nullptr;

void collectUiMessage(const string& message) {
    if (g_collectedMessages && !message.empty()) {
        g_collectedMessages->push_back(message);
    }
}

IncomingOfferDecision autoOfferDecision(const Career& career,
                                        const Player& player,
                                        long long offer,
                                        long long maxOffer) {
    IncomingOfferDecision decision;
    size_t squadSize = career.myTeam ? career.myTeam->players.size() : 0;
    if (player.wantsToLeave && offer >= player.value) {
        decision.action = 1;
        return decision;
    }
    if (squadSize > 20 && offer >= max(player.value * 12 / 10, maxOffer * 90 / 100)) {
        decision.action = 1;
        return decision;
    }
    if (offer >= maxOffer) {
        decision.action = 1;
        return decision;
    }
    decision.action = 3;
    return decision;
}

bool autoRenewDecision(const Career&,
                       const Team& team,
                       const Player& player,
                       long long demandedWage,
                       int,
                       long long) {
    if (team.budget < demandedWage * 6) return false;
    if (player.wantsToLeave && player.happiness < 45) return false;
    int averageSkill = team.getAverageSkill();
    if (player.skill >= averageSkill - 5) return true;
    return team.players.size() <= 18;
}

int autoManagerJobDecision(const Career&, const vector<Team*>& jobs) {
    return jobs.empty() ? -1 : 0;
}

ServiceResult finalizeCapturedResult(bool ok, const vector<string>& messages, const string& capturedStdout) {
    ServiceResult result;
    result.ok = ok;
    result.messages = messages;
    vector<string> legacyLines = splitOutputLines(capturedStdout);
    result.messages.insert(result.messages.end(), legacyLines.begin(), legacyLines.end());
    return result;
}

}  // namespace

ServiceResult startCareerService(Career& career,
                                 const string& divisionId,
                                 const string& teamName,
                                 const string& managerName) {
    ServiceResult result;
    career.initializeLeague(true);
    if (career.divisions.empty()) {
        result.messages.push_back("No se encontraron divisiones disponibles.");
        return result;
    }

    career.setActiveDivision(divisionId);
    if (career.activeTeams.empty()) {
        result.messages.push_back("La division seleccionada no tiene equipos.");
        return result;
    }

    Team* selectedTeam = career.activeTeams.front();
    for (Team* team : career.activeTeams) {
        if (team && team->name == teamName) {
            selectedTeam = team;
            break;
        }
    }

    career.myTeam = selectedTeam;
    career.managerName = managerName.empty() ? "Manager" : managerName;
    career.managerReputation = 50;
    career.newsFeed.clear();
    career.scoutInbox.clear();
    career.history.clear();
    career.pendingTransfers.clear();
    career.achievements.clear();
    career.currentSeason = 1;
    career.currentWeek = 1;
    career.resetSeason();

    result.ok = true;
    result.messages.push_back("Nueva carrera iniciada con " + career.myTeam->name + ".");
    return result;
}

ServiceResult loadCareerService(Career& career) {
    ServiceResult result;
    career.initializeLeague(true);
    result.ok = career.loadCareer();
    if (result.ok) {
        result.messages.push_back("Carrera cargada: " + (career.myTeam ? career.myTeam->name : string("Sin club")) + ".");
    } else {
        result.messages.push_back("No se encontro una carrera guardada.");
    }
    return result;
}

ServiceResult saveCareerService(Career& career) {
    ServiceResult result;
    result.ok = career.saveCareer();
    if (result.ok) {
        result.messages.push_back("Carrera guardada en " + career.saveFile + ".");
    } else {
        result.messages.push_back("No se pudo guardar la carrera en " + career.saveFile + ".");
    }
    return result;
}

ServiceResult simulateCareerWeekService(Career& career) {
    vector<string> messages;
    g_collectedMessages = &messages;
    setUiMessageCallback(collectUiMessage);
    setIncomingOfferDecisionCallback(autoOfferDecision);
    setContractRenewalDecisionCallback(autoRenewDecision);
    setManagerJobSelectionCallback(autoManagerJobDecision);

    StdoutCapture capture;
    simulateCareerWeek(career);

    setManagerJobSelectionCallback(nullptr);
    setContractRenewalDecisionCallback(nullptr);
    setIncomingOfferDecisionCallback(nullptr);
    setUiMessageCallback(nullptr);
    g_collectedMessages = nullptr;

    ServiceResult result = finalizeCapturedResult(true, messages, capture.str());
    if (result.messages.empty()) {
        result.messages.push_back("Semana simulada.");
    }
    result.ok = true;
    return result;
}

ValidationSuiteSummary runValidationService() {
    return buildValidationSuiteSummary();
}
