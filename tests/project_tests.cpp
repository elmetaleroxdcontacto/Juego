#include "ai/ai_transfer_manager.h"
#include "career/career_reports.h"
#include "competition/competition.h"
#include "engine/models.h"
#include "simulation/match_context.h"
#include "simulation/match_engine.h"
#include "simulation/match_phase.h"
#include "validators/validators.h"

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

void expect(bool condition, const string& message) {
    if (!condition) throw runtime_error(message);
}

string joinLines(const vector<string>& lines) {
    ostringstream out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i) out << '\n';
        out << lines[i];
    }
    return out.str();
}

Player makePlayer(const string& name,
                  const string& position,
                  int skill,
                  int potential,
                  int age,
                  int stamina,
                  int fitness) {
    Player player{};
    player.name = name;
    player.position = position;
    player.preferredFoot = "Derecho";
    player.skill = skill;
    player.potential = potential;
    player.age = age;
    player.stamina = stamina;
    player.fitness = fitness;
    player.value = static_cast<long long>(skill) * 15000;
    player.wage = static_cast<long long>(skill) * 190;
    player.releaseClause = player.value * 2;
    player.setPieceSkill = skill;
    player.leadership = 52;
    player.professionalism = 68;
    player.ambition = 61;
    player.consistency = 64;
    player.bigMatches = 63;
    player.currentForm = 58 + skill / 4;
    player.tacticalDiscipline = 62;
    player.versatility = 54;
    player.happiness = 63;
    player.chemistry = 62;
    player.desiredStarts = 1;
    player.startsThisSeason = 0;
    player.wantsToLeave = false;
    player.onLoan = false;
    player.loanWeeksRemaining = 0;
    player.contractWeeks = 104;
    player.injured = false;
    player.injuryWeeks = 0;
    player.injuryHistory = 0;
    player.yellowAccumulation = 0;
    player.seasonYellowCards = 0;
    player.seasonRedCards = 0;
    player.matchesSuspended = 0;
    player.goals = 0;
    player.assists = 0;
    player.matchesPlayed = 0;
    player.lastTrainedSeason = -1;
    player.lastTrainedWeek = -1;
    applyPositionStats(player);
    ensurePlayerProfile(player, true);
    return player;
}

Team makeTeam(const string& name,
              const string& division,
              int baseSkill,
              int pressing,
              int tempo,
              const string& tactics,
              const string& instruction,
              long long budget = 450000) {
    Team team(name);
    team.division = division;
    team.formation = "4-3-3";
    team.tactics = tactics;
    team.matchInstruction = instruction;
    team.pressingIntensity = pressing;
    team.tempo = tempo;
    team.width = 3;
    team.defensiveLine = 3;
    team.markingStyle = "Zonal";
    team.rotationPolicy = "Balanceado";
    team.trainingFocus = "Balanceado";
    team.budget = budget;
    team.morale = 60;
    team.sponsorWeekly = 35000;
    team.debt = 0;
    team.stadiumLevel = 2;
    team.trainingFacilityLevel = 2;
    team.youthFacilityLevel = 2;
    team.fanBase = 24;
    team.assistantCoach = 65;
    team.fitnessCoach = 65;
    team.scoutingChief = 65;
    team.youthCoach = 64;
    team.medicalTeam = 63;

    static const vector<string> positions = {
        "ARQ", "DEF", "DEF", "DEF", "DEF",
        "MED", "MED", "MED",
        "DEL", "DEL", "DEL",
        "ARQ", "DEF", "DEF", "MED", "MED", "DEL", "DEL"
    };

    for (size_t i = 0; i < positions.size(); ++i) {
        const int skill = baseSkill - static_cast<int>(i % 4) * 2 + (positions[i] == "DEL" ? 2 : 0);
        const int age = 20 + static_cast<int>(i % 10);
        const int stamina = 66 + static_cast<int>(i % 5) * 3;
        const int fitness = 68 + static_cast<int>(i % 4) * 4;
        team.addPlayer(makePlayer(name + "_P" + to_string(i + 1), positions[i], skill, min(95, skill + 8), age, stamina, fitness));
    }

    ensureTeamIdentity(team);
    return team;
}

void testValidationSuitePasses() {
    const ValidationSuiteSummary summary = buildValidationSuiteSummary();
    expect(summary.ok, "La suite de validacion base fallo:\n" + joinLines(summary.lines));
}

void testMatchSimulationProducesStructuredPhases() {
    Team home = makeTeam("Local Unido", "primera b", 69, 4, 4, "Pressing", "Contra-presion", 600000);
    Team away = makeTeam("Visitante FC", "primera b", 66, 2, 2, "Balanced", "Bloque bajo", 420000);

    const match_engine::MatchSimulationData data = match_engine::simulate(home, away, true, false);
    const MatchResult& result = data.result;

    expect(result.timeline.phases.size() == 6, "El partido debe quedar dividido en seis fases.");
    static const vector<pair<int, int>> expectedPhases = {{1, 15}, {16, 30}, {31, 45}, {46, 60}, {61, 75}, {76, 90}};
    for (size_t i = 0; i < expectedPhases.size(); ++i) {
        const MatchPhaseReport& phase = result.timeline.phases[i];
        expect(phase.minuteStart == expectedPhases[i].first && phase.minuteEnd == expectedPhases[i].second,
               "Los limites de fase no coinciden con la estructura esperada.");
        expect(phase.homePossessionShare + phase.awayPossessionShare == 100,
               "La posesion por fase debe sumar 100.");
    }

    expect(result.homePossession + result.awayPossession == 100, "La posesion final debe sumar 100.");
    expect(result.homeGoals == result.stats.homeGoals && result.awayGoals == result.stats.awayGoals,
           "El resultado final debe reflejar las estadisticas acumuladas.");
    expect(result.homeShots == result.stats.homeShots && result.awayShots == result.stats.awayShots,
           "Los tiros del resumen deben coincidir con MatchStats.");
    expect(result.stats.homeShotsOnTarget <= result.homeShots && result.stats.awayShotsOnTarget <= result.awayShots,
           "Los tiros al arco no pueden exceder los tiros totales.");
    expect(!result.timeline.events.empty(), "La linea de tiempo debe contener eventos.");
    expect(result.stats.homeExpectedGoals >= 0.0 && result.stats.awayExpectedGoals >= 0.0,
           "El xG no puede ser negativo.");
    expect(!result.reportLines.empty(), "El motor debe producir lineas de reporte legibles.");
}

void testHighPressRaisesPhaseFatigue() {
    Team highPress = makeTeam("Presion Alta", "primera division", 72, 5, 5, "Pressing", "Contra-presion", 800000);
    Team lowBlock = makeTeam("Bloque Bajo", "primera division", 72, 1, 2, "Defensive", "Pausar juego", 800000);
    Team opponent = makeTeam("Rival Control", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 800000);

    const MatchSetup highSetup = match_context::buildMatchSetup(highPress, opponent, false, false);
    const MatchSetup lowSetup = match_context::buildMatchSetup(lowBlock, opponent, false, false);

    const MatchPhaseEvaluation highEval =
        match_phase::evaluatePhase(highSetup, highPress, opponent, highSetup.home, highSetup.away, 0, 1, 15);
    const MatchPhaseEvaluation lowEval =
        match_phase::evaluatePhase(lowSetup, lowBlock, opponent, lowSetup.home, lowSetup.away, 0, 1, 15);

    expect(highEval.report.homeFatigueGain > lowEval.report.homeFatigueGain,
           "La presion alta debe aumentar el desgaste de fase.");
    expect(highEval.report.intensity > lowEval.report.intensity,
           "La presion alta debe elevar la intensidad del tramo.");
}

void testCompetitionGroupTableScopesActiveGroup() {
    Career career;
    career.activeDivision = "segunda division";
    static const vector<string> names = {
        "Norte Azul", "Norte Rojo", "Norte Blanco", "Norte Negro", "Norte Plata", "Norte Cobre", "Norte Mar",
        "Sur Verde", "Sur Oro", "Sur Celeste", "Sur Vino", "Sur Gris", "Sur Arena", "Sur Austral"
    };
    static const vector<int> points = {24, 18, 15, 14, 12, 11, 10, 30, 16, 13, 12, 10, 9, 8};
    for (size_t i = 0; i < names.size(); ++i) {
        career.allTeams.push_back(makeTeam(names[i], "segunda division", 62 - static_cast<int>(i % 4), 3, 3, "Balanced", "Equilibrado"));
        career.allTeams.back().points = points[i];
    }

    for (size_t i = 0; i < career.allTeams.size(); ++i) {
        career.activeTeams.push_back(&career.allTeams[i]);
        if (i < 7) career.groupNorthIdx.push_back(static_cast<int>(i));
        else career.groupSouthIdx.push_back(static_cast<int>(i));
    }
    career.myTeam = career.activeTeams[0];

    const LeagueTable northTable = buildCompetitionGroupTable(career, true);
    expect(northTable.title == "Grupo Norte", "La tabla del grupo norte debe usar el titulo correcto.");
    expect(northTable.teams.size() == 7, "La tabla de grupo debe incluir solo a los equipos del grupo solicitado.");
    expect(northTable.teams[0]->name == "Norte Azul", "La tabla del grupo debe mantener el orden competitivo.");

    const LeagueTable relevant = buildRelevantCompetitionTable(career);
    expect(relevant.teams.size() == 7 && relevant.teams[0]->name == "Norte Azul",
           "La tabla relevante debe quedarse con el grupo del club usuario.");
}

void testTransferEvaluationPenalizesUnaffordableDeals() {
    Career career;
    career.currentSeason = 4;
    career.managerReputation = 84;

    Team buyer = makeTeam("Club Comprador", "primera division", 66, 3, 3, "Balanced", "Equilibrado", 90000);
    buyer.sponsorWeekly = 60000;
    buyer.debt = 0;
    buyer.fanBase = 42;
    buyer.trainingFacilityLevel = 4;
    buyer.youthFacilityLevel = 3;
    ensureTeamIdentity(buyer);

    Team seller = makeTeam("Club Vendedor", "primera division", 74, 3, 3, "Balanced", "Equilibrado", 900000);
    ensureTeamIdentity(seller);

    ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, buyer);

    Player expensiveVeteran = makePlayer("Veterano Caro", "DEL", 81, 82, 27, 71, 72);
    expensiveVeteran.value = 900000;
    expensiveVeteran.releaseClause = 1500000;
    expensiveVeteran.wage = 52000;
    expensiveVeteran.contractWeeks = 88;
    expensiveVeteran.currentForm = 76;

    TransferTarget expensiveTarget =
        ai_transfer_manager::evaluateTarget(career, buyer, seller, expensiveVeteran, strategy);
    expect(!expensiveTarget.availableForLoan, "Un jugador de 27 anos no deberia marcarse como prestable.");
    expect(expensiveTarget.affordabilityScore < 0,
           "Una operacion muy cara debe penalizarse por presupuesto.");

    Player youngProspect = makePlayer("Proyecto Cesion", "MED", 72, 86, 20, 74, 76);
    youngProspect.value = 650000;
    youngProspect.releaseClause = 1200000;
    youngProspect.wage = 9000;
    youngProspect.contractWeeks = 104;
    youngProspect.currentForm = 61;

    TransferTarget loanTarget =
        ai_transfer_manager::evaluateTarget(career, buyer, seller, youngProspect, strategy);
    expect(loanTarget.availableForLoan, "Un juvenil con contrato largo debe quedar disponible para cesion.");
    expect(loanTarget.affordabilityScore > expensiveTarget.affordabilityScore,
           "La IA debe valorar mejor una cesion asumible que una compra imposible.");
}

}  // namespace

int main() {
    const vector<pair<string, void (*)()>> tests = {
        {"validation_suite", testValidationSuitePasses},
        {"match_engine_structure", testMatchSimulationProducesStructuredPhases},
        {"tactical_fatigue", testHighPressRaisesPhaseFatigue},
        {"competition_group_table", testCompetitionGroupTableScopesActiveGroup},
        {"transfer_affordability", testTransferEvaluationPenalizesUnaffordableDeals},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            cout << "[PASS] " << test.first << '\n';
        } catch (const exception& ex) {
            ++failures;
            cerr << "[FAIL] " << test.first << ": " << ex.what() << '\n';
        }
    }

    if (failures == 0) {
        cout << "All tests passed." << endl;
        return 0;
    }

    cerr << failures << " test(s) failed." << endl;
    return 1;
}
