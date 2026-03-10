#include "career/match_center_service.h"

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;

namespace {

string formatXgTenth(int value) {
    ostringstream out;
    out.setf(ios::fixed);
    out.precision(1);
    out << (value / 10.0);
    return out.str();
}

string phaseLeadLabel(const MatchCenterSnapshot& snapshot) {
    if (snapshot.phaseSummaries.empty()) return "Sin lectura por fases.";
    return snapshot.phaseSummaries.front();
}

}  // namespace

namespace match_center_service {

void captureLastMatchCenter(Career& career,
                            const Team& home,
                            const Team& away,
                            const MatchResult& result,
                            bool cupMatch) {
    if (!career.myTeam) return;
    const bool myHome = (&home == career.myTeam);
    const bool myAway = (&away == career.myTeam);
    if (!myHome && !myAway) return;

    MatchCenterSnapshot snapshot;
    snapshot.competitionLabel = cupMatch ? "Copa" : "Liga";
    snapshot.opponentName = myHome ? away.name : home.name;
    snapshot.venueLabel = myHome ? "Local" : "Visita";
    snapshot.myGoals = myHome ? result.homeGoals : result.awayGoals;
    snapshot.oppGoals = myHome ? result.awayGoals : result.homeGoals;
    snapshot.myShots = myHome ? result.homeShots : result.awayShots;
    snapshot.oppShots = myHome ? result.awayShots : result.homeShots;
    snapshot.myShotsOnTarget = myHome ? result.stats.homeShotsOnTarget : result.stats.awayShotsOnTarget;
    snapshot.oppShotsOnTarget = myHome ? result.stats.awayShotsOnTarget : result.stats.homeShotsOnTarget;
    snapshot.myPossession = myHome ? result.homePossession : result.awayPossession;
    snapshot.oppPossession = myHome ? result.awayPossession : result.homePossession;
    snapshot.myCorners = myHome ? result.homeCorners : result.awayCorners;
    snapshot.oppCorners = myHome ? result.awayCorners : result.homeCorners;
    snapshot.mySubstitutions = myHome ? result.homeSubstitutions : result.awaySubstitutions;
    snapshot.oppSubstitutions = myHome ? result.awaySubstitutions : result.homeSubstitutions;
    snapshot.myExpectedGoalsTenths =
        static_cast<int>(std::round((myHome ? result.stats.homeExpectedGoals : result.stats.awayExpectedGoals) * 10.0));
    snapshot.oppExpectedGoalsTenths =
        static_cast<int>(std::round((myHome ? result.stats.awayExpectedGoals : result.stats.homeExpectedGoals) * 10.0));
    snapshot.weather = result.weather;
    snapshot.dominanceSummary = result.report.explanation.likelyReason;
    snapshot.tacticalSummary = result.report.explanation.tacticalStory;
    snapshot.fatigueSummary = result.report.explanation.fatigueStory;
    snapshot.postMatchImpact = result.report.postMatchImpact;
    snapshot.phaseSummaries = result.report.phaseSummaries;
    career.lastMatchCenter = snapshot;
}

MatchCenterView buildLastMatchCenter(const Career& career,
                                     size_t maxPhases,
                                     size_t maxEvents) {
    MatchCenterView view;
    const MatchCenterSnapshot& snapshot = career.lastMatchCenter;
    if (career.lastMatchAnalysis.empty() && snapshot.opponentName.empty() &&
        career.lastMatchEvents.empty() && career.lastMatchReportLines.empty()) {
        return view;
    }

    view.available = true;
    view.headline = career.lastMatchAnalysis;
    if (!snapshot.opponentName.empty()) {
        view.scoreboard = snapshot.competitionLabel + " | " + snapshot.venueLabel + " vs " + snapshot.opponentName +
                          " | " + to_string(snapshot.myGoals) + "-" + to_string(snapshot.oppGoals) +
                          " | clima " + snapshot.weather;
    }
    view.tacticalSummary = snapshot.tacticalSummary;
    view.fatigueSummary = snapshot.fatigueSummary;
    view.postMatchImpact = snapshot.postMatchImpact;
    view.playerOfTheMatch = career.lastMatchPlayerOfTheMatch;
    if (!snapshot.opponentName.empty()) {
        view.metrics.push_back({"Tiros", to_string(snapshot.myShots), to_string(snapshot.oppShots)});
        view.metrics.push_back({"Arco", to_string(snapshot.myShotsOnTarget), to_string(snapshot.oppShotsOnTarget)});
        view.metrics.push_back({"Posesion", to_string(snapshot.myPossession) + "%", to_string(snapshot.oppPossession) + "%"});
        view.metrics.push_back({"Corners", to_string(snapshot.myCorners), to_string(snapshot.oppCorners)});
        view.metrics.push_back({"xG", formatXgTenth(snapshot.myExpectedGoalsTenths), formatXgTenth(snapshot.oppExpectedGoalsTenths)});
        view.metrics.push_back({"Cambios", to_string(snapshot.mySubstitutions), to_string(snapshot.oppSubstitutions)});
    }

    const size_t phaseCount = min(maxPhases, snapshot.phaseSummaries.size());
    for (size_t i = 0; i < phaseCount; ++i) {
        view.phaseLines.push_back(snapshot.phaseSummaries[i]);
    }

    const size_t eventCount = min(maxEvents, career.lastMatchEvents.size());
    for (size_t i = 0; i < eventCount; ++i) {
        view.eventLines.push_back(career.lastMatchEvents[i]);
    }
    return view;
}

string formatLastMatchCenter(const Career& career,
                             size_t maxPhases,
                             size_t maxEvents) {
    MatchCenterView view = buildLastMatchCenter(career, maxPhases, maxEvents);
    if (!view.available) return "No hay match center disponible.";

    ostringstream out;
    out << "MatchCenter\r\n";
    if (!view.scoreboard.empty()) out << view.scoreboard << "\r\n";
    if (!view.headline.empty()) out << view.headline << "\r\n";
    if (!view.playerOfTheMatch.empty()) out << "Figura: " << view.playerOfTheMatch << "\r\n";
    if (!view.metrics.empty()) {
        out << "\r\nMetricas\r\n";
        for (const MatchCenterMetric& metric : view.metrics) {
            out << "- " << metric.label << ": " << metric.myValue << " / " << metric.oppValue << "\r\n";
        }
    }
    if (!view.tacticalSummary.empty()) out << "\r\nTactica\r\n- " << view.tacticalSummary << "\r\n";
    if (!view.fatigueSummary.empty()) out << "Fatiga\r\n- " << view.fatigueSummary << "\r\n";
    if (!view.postMatchImpact.empty()) out << "Impacto\r\n- " << view.postMatchImpact << "\r\n";
    if (!view.phaseLines.empty()) {
        out << "\r\nFases\r\n";
        for (const string& line : view.phaseLines) out << "- " << line << "\r\n";
    } else if (!career.lastMatchCenter.phaseSummaries.empty()) {
        out << "\r\nFases\r\n- " << phaseLeadLabel(career.lastMatchCenter) << "\r\n";
    }
    if (!view.eventLines.empty()) {
        out << "\r\nTimeline\r\n";
        for (const string& event : view.eventLines) out << "- " << event << "\r\n";
    }
    return out.str();
}

}  // namespace match_center_service
