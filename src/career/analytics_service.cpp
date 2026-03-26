#include "career/analytics_service.h"

#include "simulation/player_condition.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

namespace {

string formatTenthsValue(int tenths) {
    ostringstream out;
    out << fixed << setprecision(1) << (tenths / 10.0);
    return out.str();
}

int countPreferredStarters(const Team& team) {
    int continuity = 0;
    for (const auto& name : team.preferredXI) {
        for (const auto& player : team.players) {
            if (player.name == name && !player.injured && player.fitness >= 55) {
                continuity++;
                break;
            }
        }
    }
    return continuity;
}

}  // namespace

namespace analytics_service {

TeamAnalyticsSnapshot buildTeamAnalyticsSnapshot(const Career& career, const Team& team) {
    TeamAnalyticsSnapshot snapshot;
    int goalkeepers = 0;
    int defenders = 0;
    int midfielders = 0;
    int forwards = 0;

    for (const auto& player : team.players) {
        const string pos = normalizePosition(player.position);
        snapshot.attackIndex += player.attack + player.currentForm / 2 + (normalizePosition(player.promisedPosition) == pos ? 1 : 0);
        snapshot.controlIndex += player.skill + player.tacticalDiscipline / 2 + player.chemistry / 4;
        snapshot.defenseIndex += player.defense + player.consistency / 2 + max(0, player.fitness - 50) / 3;
        snapshot.setPieceThreat += player.setPieceSkill / 3;
        snapshot.aerialThreat += (player.defense + player.attack + player.leadership) / 6;
        if (player.contractWeeks <= 18) snapshot.contractRisk++;
        if (player_condition::workloadRisk(player, team) >= 55) snapshot.fatigueRisk++;
        if (player.age <= 21 && player.potential >= player.skill + 8) snapshot.youthUpside++;
        if (player.startsThisSeason >= max(1, career.currentWeek / 3)) snapshot.roleBalance += 2;
        if (player.promisedRole == "Titular" && player.startsThisSeason == 0) snapshot.roleBalance -= 2;
        if (player.position == "ARQ") goalkeepers++;
        else if (pos == "DEF") defenders++;
        else if (pos == "MED") midfielders++;
        else if (pos == "DEL") forwards++;
    }

    snapshot.continuityScore = countPreferredStarters(team) * 10;
    snapshot.roleBalance += min(defenders, 4) * 2 + min(midfielders, 4) * 2 + min(forwards, 3) * 2 + min(goalkeepers, 2) * 3;
    snapshot.roleBalance = clampInt(snapshot.roleBalance, 0, 100);

    if (career.myTeam == &team) {
        snapshot.xgForTenths = career.lastMatchCenter.myExpectedGoalsTenths;
        snapshot.xgAgainstTenths = career.lastMatchCenter.oppExpectedGoalsTenths;
        snapshot.finishingDeltaTenths = (career.lastMatchCenter.myGoals * 10 - snapshot.xgForTenths) -
                                        (career.lastMatchCenter.oppGoals * 10 - snapshot.xgAgainstTenths);
    }

    if (snapshot.fatigueRisk >= 5) snapshot.pressureWindow = "Riesgo fisico alto";
    else if (snapshot.contractRisk >= 4) snapshot.pressureWindow = "Varios contratos bajo presion";
    else if (snapshot.roleBalance <= 42) snapshot.pressureWindow = "Balance de roles fragil";
    else snapshot.pressureWindow = "Semana controlada";

    if (snapshot.fatigueRisk >= 5) snapshot.alerts.push_back("La carga fisica exige mas rotacion esta semana.");
    if (snapshot.contractRisk >= 4) snapshot.alerts.push_back("Hay demasiados contratos cerca de zona critica.");
    if (snapshot.youthUpside >= 4) snapshot.alerts.push_back("Existe una ola juvenil lista para absorber minutos.");
    if (snapshot.roleBalance <= 42) snapshot.alerts.push_back("Faltan perfiles equilibrados entre lineas.");
    if (snapshot.xgAgainstTenths >= snapshot.xgForTenths + 5) snapshot.alerts.push_back("El ultimo partido dejo al equipo concediendo mas de lo que genero.");
    if (snapshot.alerts.empty()) snapshot.alerts.push_back("Los indicadores del club se mantienen en una franja estable.");
    return snapshot;
}

vector<string> buildTeamAnalyticsLines(const Career& career, const Team& team) {
    const TeamAnalyticsSnapshot snapshot = buildTeamAnalyticsSnapshot(career, team);
    return {
        "Indice ofensivo: " + to_string(snapshot.attackIndex),
        "Indice de control: " + to_string(snapshot.controlIndex),
        "Indice defensivo: " + to_string(snapshot.defenseIndex),
        "Continuidad del XI: " + to_string(snapshot.continuityScore) + "/110",
        "Balance de roles: " + to_string(snapshot.roleBalance) + "/100",
        "Balon parado: " + to_string(snapshot.setPieceThreat) + " | juego aereo " + to_string(snapshot.aerialThreat),
        "Riesgos contractuales: " + to_string(snapshot.contractRisk) + " | fatiga " + to_string(snapshot.fatigueRisk),
        "Pipeline juvenil: " + to_string(snapshot.youthUpside),
        "Ventana actual: " + snapshot.pressureWindow
    };
}

vector<string> buildMatchTrendLines(const Career& career) {
    vector<string> lines;
    if (career.lastMatchCenter.opponentName.empty()) return lines;
    const MatchCenterSnapshot& match = career.lastMatchCenter;
    lines.push_back("Ultimo partido: " + match.opponentName + " | " + to_string(match.myGoals) + "-" + to_string(match.oppGoals));
    lines.push_back("xG: " + formatTenthsValue(match.myExpectedGoalsTenths) + " vs " + formatTenthsValue(match.oppExpectedGoalsTenths));
    lines.push_back("Tiros: " + to_string(match.myShots) + "-" + to_string(match.oppShots) +
                    " | Posesion: " + to_string(match.myPossession) + "%");
    lines.push_back("Lectura tactica: " + (match.tacticalSummary.empty() ? string("sin dato") : match.tacticalSummary));
    lines.push_back("Impacto fisico: " + (match.fatigueSummary.empty() ? string("sin dato") : match.fatigueSummary));
    return lines;
}

}  // namespace analytics_service
