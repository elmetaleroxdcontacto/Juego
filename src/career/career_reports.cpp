#include "career/career_reports.h"

#include "ai/ai_squad_planner.h"
#include "career/analytics_service.h"
#include "career/dressing_room_service.h"
#include "career/inbox_service.h"
#include "career/medical_service.h"
#include "career/staff_service.h"
#include "career/match_center_service.h"
#include "career/match_analysis_store.h"
#include "career/career_support.h"
#include "career/team_management.h"
#include "career/world_state_service.h"
#include "competition/competition.h"
#include "development/training_impact_system.h"
#include "finance/finance_system.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

LeagueTable buildTableFromTeams(const vector<Team*>& teams, const string& title, const string& ruleId) {
    LeagueTable table;
    table.title = title;
    table.ruleId = ruleId;
    for (Team* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

int competitionGroupForTeamInternal(const Career& career, const Team* team) {
    for (int index : career.groupNorthIdx) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) &&
            career.activeTeams[static_cast<size_t>(index)] == team) {
            return 0;
        }
    }
    for (int index : career.groupSouthIdx) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size()) &&
            career.activeTeams[static_cast<size_t>(index)] == team) {
            return 1;
        }
    }
    return -1;
}

void addFact(CareerReport& report, const string& label, const string& value) {
    report.facts.push_back({label, value});
}

void addBlock(CareerReport& report, const string& title, vector<string> lines) {
    if (lines.empty()) return;
    report.blocks.push_back({title, std::move(lines)});
}

vector<string> activePromiseLines(const Career& career, size_t limit = 5) {
    vector<string> lines;
    const size_t count = min(limit, career.activePromises.size());
    for (size_t i = 0; i < count; ++i) {
        const SquadPromise& promise = career.activePromises[i];
        lines.push_back(promise.category + " | " + promise.subjectName + " | " + promise.target +
                        " | progreso " + to_string(promise.progress) +
                        " | vence F" + to_string(promise.deadlineWeek));
    }
    return lines;
}

vector<string> historicalRecordLines(const Career& career, size_t limit = 4) {
    vector<string> lines;
    if (career.historicalRecords.empty()) return lines;
    const size_t start = career.historicalRecords.size() > limit ? career.historicalRecords.size() - limit : 0;
    for (size_t i = start; i < career.historicalRecords.size(); ++i) {
        const HistoricalRecord& record = career.historicalRecords[i];
        lines.push_back(record.category + " | " + record.holderName + " (" + record.teamName + ")" +
                        " | " + to_string(record.value) + " | T" + to_string(record.season));
    }
    return lines;
}

vector<string> plannerLines(const Team& team) {
    const SquadNeedReport planner = ai_squad_planner::analyzeSquad(team);
    return {
        "Necesidad principal: " + planner.weakestPosition + " | riesgo de rotacion " + to_string(planner.rotationRisk),
        "Exceso relativo: " + planner.surplusPosition + " | presion de venta " + to_string(planner.salePressure),
        "Posiciones finas: " + (planner.thinPositions.empty() ? string("-") : joinStringValues(planner.thinPositions, ", ")),
        "Cobertura juvenil: " + (planner.youthCoverPositions.empty() ? string("-") : joinStringValues(planner.youthCoverPositions, ", ")),
        "Candidatos de salida: " + (planner.saleCandidates.empty() ? string("-") : joinStringValues(planner.saleCandidates, ", "))
    };
}

vector<string> scoutingAssignmentLines(const Career& career) {
    vector<string> lines;
    for (const auto& assignment : career.scoutingAssignments) {
        lines.push_back(assignment.region + " | foco " + assignment.focusPosition +
                        " | prioridad " + assignment.priority +
                        " | conocimiento " + to_string(assignment.knowledgeLevel) +
                        "% | " + to_string(assignment.weeksRemaining) + " sem");
    }
    return lines;
}

vector<string> staffDigestLines(const Team& team) {
    vector<string> lines;
    for (const auto& profile : staff_service::buildStaffProfiles(team)) {
        lines.push_back(profile.role + " | " + profile.name +
                        " | nivel " + to_string(profile.rating) +
                        " | " + profile.status +
                        " | costo " + formatMoneyValue(profile.weeklyCost));
    }
    return lines;
}

vector<string> medicalDigestLines(const Team& team) {
    vector<string> lines;
    for (const auto& status : medical_service::buildMedicalStatuses(team)) {
        lines.push_back(status.playerName + " | " + status.diagnosis +
                        " | carga " + to_string(status.workloadRisk) +
                        " | recaida " + to_string(status.relapseRisk) +
                        " | " + status.recommendation);
    }
    if (lines.empty()) lines.push_back("Centro medico sin casos criticos.");
    return lines;
}


}  // namespace

string formatMoneyValue(long long value) {
    bool negative = value < 0;
    unsigned long long absValue = static_cast<unsigned long long>(negative ? -value : value);
    string digits = to_string(absValue);
    string out;
    for (size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) out += ',';
        out += digits[i];
    }
    return string(negative ? "-$" : "$") + out;
}

string detectScoutingNeed(const Team& team) {
    static const vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    string best = "MED";
    int bestScore = 1000000;
    for (const auto& pos : positions) {
        int count = 0;
        int totalSkill = 0;
        for (const auto& player : team.players) {
            if (normalizePosition(player.position) != pos) continue;
            count++;
            totalSkill += player.skill;
        }
        int average = count > 0 ? totalSkill / count : 0;
        int score = count * 20 + average;
        if (score < bestScore) {
            bestScore = score;
            best = pos;
        }
    }
    return best;
}

int competitionGroupForTeam(const Career& career, const Team* team) {
    return competitionGroupForTeamInternal(career, team);
}

LeagueTable buildCompetitionGroupTable(const Career& career, bool north) {
    const vector<int>& indices = north ? career.groupNorthIdx : career.groupSouthIdx;
    vector<Team*> teams;
    for (int index : indices) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[static_cast<size_t>(index)]);
        }
    }
    return buildTableFromTeams(teams, competitionGroupTitle(career.activeDivision, north), career.activeDivision);
}

LeagueTable buildRelevantCompetitionTable(const Career& career) {
    if (!career.myTeam) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }
    if (!career.usesGroupFormat() || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }

    vector<Team*> teams;
    const vector<int>* indices = nullptr;
    int group = competitionGroupForTeamInternal(career, career.myTeam);
    if (group == 0) indices = &career.groupNorthIdx;
    else if (group == 1) indices = &career.groupSouthIdx;
    if (!indices) {
        LeagueTable table = career.leagueTable;
        table.sortTable();
        return table;
    }

    for (int index : *indices) {
        if (index >= 0 && index < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[static_cast<size_t>(index)]);
        }
    }
    string title = competitionGroupTitle(career.activeDivision, group == 0);
    return buildTableFromTeams(teams, title, career.activeDivision);
}

CareerReport buildCompetitionReport(const Career& career) {
    CareerReport report;
    report.title = "Competicion";
    if (!career.myTeam) {
        addFact(report, "Estado", "No hay carrera activa.");
        return report;
    }

    LeagueTable table = buildRelevantCompetitionTable(career);
    addFact(report, "Division", divisionDisplay(career.activeDivision));
    addFact(report, "Temporada", to_string(career.currentSeason));
    addFact(report, "Fecha", to_string(career.currentWeek) + "/" + to_string(max(1, static_cast<int>(career.schedule.size()))));
    addFact(report, "Club", career.myTeam->name);
    addFact(report, "Posicion", to_string(career.currentCompetitiveRank()) + "/" + to_string(max(1, career.currentCompetitiveFieldSize())));
    addFact(report, "Puntos", to_string(career.myTeam->points));
    addFact(report, "DG", to_string(career.myTeam->goalsFor - career.myTeam->goalsAgainst));

    if (!table.teams.empty()) {
        vector<string> topLines;
        int limit = min(5, static_cast<int>(table.teams.size()));
        for (int i = 0; i < limit; ++i) {
            Team* team = table.teams[static_cast<size_t>(i)];
            string line = to_string(i + 1) + ". " + team->name + " | " + to_string(team->points) + " pts";
            if (team == career.myTeam) line += " <- tu club";
            topLines.push_back(line);
        }
        addBlock(report, "Top 5", std::move(topLines));

        if (static_cast<int>(table.teams.size()) > 5) {
            vector<string> bottomLines;
            int start = max(0, static_cast<int>(table.teams.size()) - 3);
            for (int i = start; i < static_cast<int>(table.teams.size()); ++i) {
                Team* team = table.teams[static_cast<size_t>(i)];
                bottomLines.push_back(to_string(i + 1) + ". " + team->name + " | " + to_string(team->points) + " pts");
            }
            addBlock(report, "Zona baja", std::move(bottomLines));
        }
    }

    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        vector<string> fixtures;
        for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
            if (match.first < 0 || match.second < 0 ||
                match.first >= static_cast<int>(career.activeTeams.size()) ||
                match.second >= static_cast<int>(career.activeTeams.size())) {
                continue;
            }
            Team* home = career.activeTeams[static_cast<size_t>(match.first)];
            Team* away = career.activeTeams[static_cast<size_t>(match.second)];
            string line = home->name + " vs " + away->name;
            if (home == career.myTeam || away == career.myTeam) line += " <- tu partido";
            fixtures.push_back(line);
        }
        addBlock(report, "Proxima fecha", std::move(fixtures));
        addBlock(report, "Informe rival", {buildOpponentReport(career)});
    }

    if (!career.newsFeed.empty()) {
        vector<string> news;
        int start = max(0, static_cast<int>(career.newsFeed.size()) - 3);
        for (size_t i = static_cast<size_t>(start); i < career.newsFeed.size(); ++i) {
            news.push_back(career.newsFeed[i]);
        }
        addBlock(report, "Ultimas noticias", std::move(news));
    }
    addBlock(report, "Records historicos", historicalRecordLines(career));
    MatchCenterView matchCenter = match_center_service::buildLastMatchCenter(career, 3, 4);
    if (matchCenter.available) {
        vector<string> lastMatchBlock;
        if (!matchCenter.scoreboard.empty()) lastMatchBlock.push_back(matchCenter.scoreboard);
        if (!matchCenter.tacticalSummary.empty()) lastMatchBlock.push_back(matchCenter.tacticalSummary);
        if (!matchCenter.fatigueSummary.empty()) lastMatchBlock.push_back(matchCenter.fatigueSummary);
        if (!matchCenter.playerOfTheMatch.empty()) lastMatchBlock.push_back("Figura: " + matchCenter.playerOfTheMatch);
        addBlock(report, "Ultimo analisis", std::move(lastMatchBlock));
    }
    if (!career.cupChampion.empty()) {
        addBlock(report, "Copa", {"Campeon: " + career.cupChampion});
    } else if (career.cupActive) {
        addBlock(report, "Copa", {"Ronda " + to_string(career.cupRound + 1) +
                                  " | Equipos vivos " + to_string(career.cupRemainingTeams.size())});
    }
    return report;
}

CareerReport buildBoardReport(const Career& career) {
    CareerReport report;
    report.title = "Directiva";
    if (!career.myTeam) {
        addFact(report, "Estado", "No hay carrera activa.");
        return report;
    }

    int youthUsed = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.age <= 20 && player.matchesPlayed > 0) youthUsed++;
    }
    vector<Team*> jobs = buildJobMarket(career);
    DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(*career.myTeam, career.currentWeek);

    addFact(report, "Confianza", boardStatusLabel(career.boardConfidence) + " (" + to_string(career.boardConfidence) + "/100)");
    addFact(report, "Reputacion manager", to_string(career.managerReputation) + "/100");
    addFact(report, "Perfil del DT", "juego " + managerStyleLabel(*career.myTeam));
    addFact(report, "Estabilidad del club", to_string(career.myTeam->jobSecurity) + "/100");
    addFact(report, "DT del club", career.myTeam->headCoachName);
    addFact(report, "Advertencias", to_string(career.boardWarningWeeks));
    addFact(report, "Vestuario", dressingRoomClimate(*career.myTeam));
    addFact(report, "Promesas en riesgo", to_string(promisesAtRisk(*career.myTeam, career.currentWeek)));
    addFact(report, "Promesas activas", to_string(career.activePromises.size()));
    addFact(report, "Asignaciones scouting", to_string(career.scoutingAssignments.size()));
    addFact(report, "Tension social", to_string(dressing.socialTension));
    addFact(report, "Apoyo de lideres", to_string(dressing.leadershipSupport));
    addFact(report, "Expectativa del club", teamExpectationLabel(*career.myTeam));
    addFact(report, "Prestigio", to_string(teamPrestigeScore(*career.myTeam)));

    addBlock(report,
             "Objetivos de temporada",
             {
                 "Puesto esperado: " + to_string(career.boardExpectedFinish) + " | actual " + to_string(career.currentCompetitiveRank()),
                 "Presupuesto meta: " + formatMoneyValue(career.boardBudgetTarget) + " | actual " + formatMoneyValue(career.myTeam->budget),
                 "Juveniles con minutos: " + to_string(youthUsed) + "/" + to_string(career.boardYouthTarget),
             });

    if (!career.boardMonthlyObjective.empty()) {
        addBlock(report,
                 "Objetivo mensual",
                 {
                     career.boardMonthlyObjective,
                     "Progreso " + to_string(career.boardMonthlyProgress) + "/" + to_string(career.boardMonthlyTarget),
                     "Semanas restantes: " + to_string(max(0, career.boardMonthlyDeadlineWeek - career.currentWeek)),
                 });
    }

    if (career.boardConfidence < 20) addBlock(report, "Estado", {"La directiva considera un ultimatum deportivo."});
    else if (career.boardConfidence < 35) addBlock(report, "Estado", {"El cargo esta bajo presion."});
    else addBlock(report, "Estado", {"Situacion controlada."});

    addBlock(report, "Vestuario", dressing.alerts);
    addBlock(report, "Grupos del vestuario", dressing.groups);
    addBlock(report, "Promesas activas", activePromiseLines(career));
    addBlock(report, "Centro del manager", inbox_service::buildInboxSummaryLines(career));
    addBlock(report, "Staff tecnico", staffDigestLines(*career.myTeam));

    vector<string> offers;
    for (Team* job : jobs) {
        offers.push_back(job->name + " (" + divisionDisplay(job->division) + ")");
    }
    addBlock(report, "Mercado de manager", std::move(offers));
    return report;
}

CareerReport buildClubReport(const Career& career) {
    CareerReport report;
    report.title = "Club y Finanzas";
    if (!career.myTeam) {
        addFact(report, "Estado", "No hay carrera activa.");
        return report;
    }

    const Team& team = *career.myTeam;
    int youthProjects = 0;
    for (const auto& player : team.players) {
        if (player.age <= 20 && player.potential - player.skill >= 8) youthProjects++;
    }
    vector<const Player*> leaders = dressingRoomLeaders(team);
    WeeklyFinanceReport finance = finance_system::projectWeeklyReport(team);
    DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(team, career.currentWeek);
    int fatigueLoad = 0;
    for (const auto& player : team.players) fatigueLoad += player.fatigueLoad;
    const int avgFatigueLoad = team.players.empty() ? 0 : fatigueLoad / static_cast<int>(team.players.size());

    addFact(report, "Presupuesto", formatMoneyValue(team.budget));
    addFact(report, "Deuda", formatMoneyValue(team.debt));
    addFact(report, "Sponsor semanal", formatMoneyValue(team.sponsorWeekly));
    addFact(report, "Masa salarial", formatMoneyValue(finance.wageBill));
    addFact(report, "Merch semanal", formatMoneyValue(finance.merchandisingIncome));
    addFact(report, "Bonos variables", formatMoneyValue(finance.bonusIncome));
    addFact(report, "Flujo semanal", formatMoneyValue(finance.netCashFlow));
    addFact(report, "Buffer de mercado", formatMoneyValue(finance.transferBuffer));
    addFact(report, "Riesgo", finance.riskLevel);
    addFact(report, "Prestigio", to_string(teamPrestigeScore(team)));
    addFact(report, "DT principal", team.headCoachName.empty() ? "Sin registro" : team.headCoachName);
    addFact(report, "Estilo del DT", team.headCoachStyle.empty() ? "Equilibrado" : team.headCoachStyle);
    addFact(report, "Antiguedad del DT", to_string(team.headCoachTenureWeeks) + " sem");
    addFact(report, "Seguridad del cargo", to_string(team.jobSecurity) + "/100");
    addFact(report, "Politica de mercado", team.transferPolicy.empty() ? "Mixta" : team.transferPolicy);
    addFact(report, "Identidad", team.clubStyle.empty() ? "Equilibrio competitivo" : team.clubStyle);
    addFact(report, "Cantera", team.youthIdentity.empty() ? "Talento local" : team.youthIdentity);
    addFact(report, "Rival principal", team.primaryRival.empty() ? "Sin clasico definido" : team.primaryRival);
    addFact(report, "Expectativa", teamExpectationLabel(team));
    addFact(report, "Vestuario", dressingRoomClimate(team));
    addFact(report, "Tension social", to_string(dressing.socialTension));
    addFact(report, "Apoyo de lideres", to_string(dressing.leadershipSupport));
    addFact(report, "Instruccion", team.matchInstruction);
    addFact(report, "Proyectos juveniles", to_string(youthProjects));
    addFact(report, "Carga acumulada", to_string(avgFatigueLoad) + "/100");
    addFact(report, "Plan semanal", team.trainingFocus);

    if (!leaders.empty()) {
        vector<string> leaderNames;
        for (const Player* leader : leaders) {
            if (leader) leaderNames.push_back(leader->name);
        }
        addBlock(report, "Nucleo de lideres", {joinStringValues(leaderNames, ", ")});
    }
    addBlock(report, "Vestuario", dressing.alerts);
    addBlock(report, "Grupos sociales", dressing.groups);
    addBlock(report, "Promesas activas", activePromiseLines(career));
    addBlock(report, "Planner de plantilla", plannerLines(team));
    addBlock(report, "Centro medico", medicalDigestLines(team));
    addBlock(report, "Data hub", analytics_service::buildTeamAnalyticsLines(career, team));
    addBlock(report, "Centro del manager", inbox_service::buildInboxSummaryLines(career));
    addBlock(report, "Staff tecnico", staffDigestLines(*career.myTeam));
    addBlock(report, "Tendencias de partido", analytics_service::buildMatchTrendLines(career));
    {
        const bool congestedWeek = career.cupActive &&
                                   (career.currentWeek == 1 || career.currentWeek % 4 == 0 ||
                                    career.currentWeek == static_cast<int>(career.schedule.size()));
        vector<string> microcycle;
        for (const auto& session : development::buildWeeklyTrainingSchedule(team, congestedWeek)) {
            microcycle.push_back(session.day + " | " + session.focus + " | carga " + to_string(session.load) +
                                 " | " + session.note);
        }
        addBlock(report, "Microciclo semanal", std::move(microcycle));
    }

    addBlock(report,
             "Infraestructura",
             {
                 "Estadio " + to_string(team.stadiumLevel),
                 "Cantera " + to_string(team.youthFacilityLevel),
                 "Entrenamiento " + to_string(team.trainingFacilityLevel),
                 "Scouting " + to_string(team.scoutingChief) + " | " + team.scoutingChiefName,
                 "Medico " + to_string(team.medicalTeam) + " | " + team.medicalChiefName,
                 "Asistente " + to_string(team.assistantCoach) + " | " + team.assistantCoachName,
                 "Preparador fisico " + to_string(team.fitnessCoach) + " | " + team.fitnessCoachName,
                 "Entrenador arqueros " + to_string(team.goalkeepingCoach) + " | " + team.goalkeepingCoachName,
                 "Analista " + to_string(team.performanceAnalyst) + " | " + team.performanceAnalystName,
                 "Jefe juveniles " + to_string(team.youthCoach) + " | " + team.youthCoachName,
                 "Region juvenil: " + team.youthRegion,
                 "Red scouting: " + (team.scoutingRegions.empty() ? string("-") : joinStringValues(team.scoutingRegions, ", ")),
             });
    addBlock(report, "Staff tecnico", staffDigestLines(team));
    return report;
}

CareerReport buildMatchCenterReport(const Career& career) {
    CareerReport report;
    report.title = "Match Center";
    MatchCenterView center = match_center_service::buildLastMatchCenter(career, 5, 8);
    if (!center.available) {
        addFact(report, "Estado", "No hay ultimo partido almacenado.");
        return report;
    }

    addFact(report, "Partido", center.scoreboard.empty() ? "Ultimo partido" : center.scoreboard);
    addFact(report, "Figura", center.playerOfTheMatch.empty() ? "Sin figura" : center.playerOfTheMatch);

    vector<string> metricLines;
    for (const MatchCenterMetric& metric : center.metrics) {
        metricLines.push_back(metric.label + ": " + metric.myValue + " / " + metric.oppValue);
    }
    addBlock(report, "Metricas", std::move(metricLines));
    addBlock(report, "Tactica", {center.tacticalSummary});
    addBlock(report, "Fatiga", {center.fatigueSummary});
    addBlock(report, "Impacto", {center.postMatchImpact});
    addBlock(report, "Fases", center.phaseLines);
    addBlock(report, "Timeline", center.eventLines);
    return report;
}

CareerReport buildScoutingReport(const Career& career) {
    CareerReport report;
    report.title = "Scouting";
    if (!career.myTeam) {
        addFact(report, "Estado", "No hay carrera activa.");
        return report;
    }

    const Team& team = *career.myTeam;
    addFact(report, "Jefe de scouting", to_string(team.scoutingChief) + "/99");
    addFact(report, "Analista", to_string(team.performanceAnalyst) + "/99");
    addFact(report, "Costo de informe", formatMoneyValue(max(3000LL, 9000LL - team.scoutingChief * 50LL)));
    addFact(report, "Necesidad detectada", detectScoutingNeed(team));
    addFact(report, "Informes guardados", to_string(career.scoutInbox.size()));
    addFact(report, "Shortlist activa", to_string(career.scoutingShortlist.size()));
    addFact(report, "Asignaciones activas", to_string(career.scoutingAssignments.size()));
    addFact(report, "Red juvenil", team.youthRegion + " | coach " + to_string(team.youthCoach));
    addFact(report, "Cobertura scouting", team.scoutingRegions.empty() ? "-" : joinStringValues(team.scoutingRegions, ", "));

    if (!career.scoutingShortlist.empty()) {
        vector<string> shortlistLines;
        for (const auto& item : career.scoutingShortlist) {
            auto fields = splitByDelimiter(item, '|');
            if (fields.size() < 2) continue;
            const Team* seller = career.findTeamByName(fields[0]);
            if (!seller) continue;
            int sellerIdx = team_mgmt::playerIndexByName(*seller, fields[1]);
            if (sellerIdx < 0) continue;
            const Player& player = seller->players[static_cast<size_t>(sellerIdx)];
            shortlistLines.push_back(player.name + " (" + seller->name + ")" +
                                     " | Contrato " + to_string(player.contractWeeks) +
                                     " | Valor " + formatMoneyValue(player.value) +
                                     " | Pie " + player.preferredFoot +
                                     " | Forma " + playerFormLabel(player) +
                                     " | Fiabilidad " + playerReliabilityLabel(player) +
                                     " | Rasgos " + joinStringValues(player.traits, ", ") +
                                     " | Perfil " + personalityLabel(player));
        }
        addBlock(report, "Shortlist", std::move(shortlistLines));
    }

    addBlock(report, "Centro del manager", inbox_service::buildInboxSummaryLines(career, 4));
    addBlock(report, "Asignaciones scouting", scoutingAssignmentLines(career));

    if (!career.scoutInbox.empty()) {
        vector<string> notes;
        int start = max(0, static_cast<int>(career.scoutInbox.size()) - 10);
        for (size_t i = static_cast<size_t>(start); i < career.scoutInbox.size(); ++i) {
            notes.push_back(career.scoutInbox[i]);
        }
        addBlock(report, "Ultimos informes", std::move(notes));
    }
    addBlock(report, "Records historicos", historicalRecordLines(career, 3));
    return report;
}

string formatCareerReport(const CareerReport& report) {
    ostringstream out;
    out << "=== " << report.title << " ===\r\n";
    for (const auto& fact : report.facts) {
        out << fact.label << ": " << fact.value << "\r\n";
    }
    for (const auto& block : report.blocks) {
        if (block.lines.empty()) continue;
        out << "\r\n" << block.title << ":\r\n";
        for (const auto& line : block.lines) {
            out << "- " << line << "\r\n";
        }
    }
    return out.str();
}
