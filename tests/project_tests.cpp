#include "ai/ai_squad_planner.h"
#include "ai/ai_transfer_manager.h"
#include "career/career_reports.h"
#include "career/dressing_room_service.h"
#include "career/match_analysis_store.h"
#include "career/match_center_service.h"
#include "career/career_support.h"
#include "career/season_service.h"
#include "career/season_transition.h"
#include "career/world_state_service.h"
#include "competition/competition.h"
#include "engine/models.h"
#include "io/io.h"
#include "simulation/match_context.h"
#include "simulation/match_engine.h"
#include "simulation/match_phase.h"
#include "simulation/simulation.h"
#include "transfers/negotiation_system.h"
#include "transfers/transfer_market.h"
#include "utils/utils.h"
#include "validators/validators.h"

#include <algorithm>
#include <exception>
#include <cstdio>
#include <fstream>
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

void testValidationSuiteReflectsRosterAudit() {
    const DataValidationReport report = buildRosterDataValidationReport();
    const ValidationSuiteSummary summary = buildValidationSuiteSummary();
    if (report.errorCount > 0) {
        expect(!summary.ok, "La suite debe fallar si la auditoria de datos detecta errores.");
        expect(joinLines(summary.lines).find("error(es) de datos") != string::npos,
               "El resumen de validacion debe informar errores de datos.");
    }
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
        expect(phase.homeProgressions >= phase.homeAttacks && phase.awayProgressions >= phase.awayAttacks,
               "Una fase no puede generar mas ataques que progresiones.");
        expect(phase.homeAttacks >= phase.homeShotsGenerated && phase.awayAttacks >= phase.awayShotsGenerated,
               "No puede haber mas remates que ataques maduros en una fase.");
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
    expect(result.report.phaseSummaries.size() == 6, "El reporte debe resumir las seis fases del partido.");
    expect(!result.report.explanation.likelyReason.empty(),
           "El reporte del partido debe incluir una explicacion probable.");
    expect(!result.report.playerOfTheMatch.empty(),
           "El reporte del partido debe identificar una figura del partido.");
    const string reportText = joinLines(result.reportLines);
    expect(reportText.find("Riesgo tactico:") != string::npos,
           "El resumen del partido debe exponer el riesgo tactico.");
    expect(reportText.find("Disciplina:") != string::npos,
           "El resumen del partido debe explicar el impacto disciplinario.");
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

void testLowBlockSuppressesChanceQuality() {
    Team home = makeTeam("Control Interior", "primera division", 72, 3, 3, "Balanced", "Equilibrado", 700000);
    Team lowBlock = makeTeam("Muralla", "primera division", 68, 1, 2, "Defensive", "Bloque bajo", 680000);
    Team openGame = makeTeam("Abierto", "primera division", 68, 4, 4, "Offensive", "Por bandas", 680000);

    const MatchSetup lowBlockSetup = match_context::buildMatchSetup(home, lowBlock, false, false);
    const MatchSetup openSetup = match_context::buildMatchSetup(home, openGame, false, false);

    const MatchPhaseEvaluation lowBlockEval =
        match_phase::evaluatePhase(lowBlockSetup, home, lowBlock, lowBlockSetup.home, lowBlockSetup.away, 1, 16, 30);
    const MatchPhaseEvaluation openEval =
        match_phase::evaluatePhase(openSetup, home, openGame, openSetup.home, openSetup.away, 1, 16, 30);

    expect(lowBlockEval.report.homeChanceProbability < openEval.report.homeChanceProbability,
           "Un bloque bajo debe recortar la calidad media de las llegadas rivales.");
    expect(lowBlockEval.report.homeDefensiveRisk < openEval.report.homeDefensiveRisk,
           "El local no deberia quedar mas expuesto ante un rival de bloque bajo que ante uno abierto.");
}

void testCompetitionRulesLoadFromCsv() {
    expect(reloadCompetitionConfigs(), "Las reglas de competicion deben poder cargarse desde competition_rules.csv.");
    const CompetitionConfig& primera = getCompetitionConfig("primera division");
    const CompetitionConfig& terceraB = getCompetitionConfig("tercera division b");

    expect(primera.baseIncome == 60000, "La Primera Division debe leer ingresos base desde el CSV externo.");
    expect(terceraB.groups.enabled && terceraB.groups.groupSize == 14,
           "La Tercera B debe conservar su configuracion zonal desde el CSV.");
    expect(competitionConfigWarnings().empty(),
           "El archivo de reglas externas no deberia generar advertencias en el escenario base.");
}

void testRuntimeValidationFlagsBrokenLoadedSquad() {
    Career career;
    career.divisions.push_back({"primera division", "data/LigaChilena/primera division", "Primera Division"});

    Team broken("Plantel Roto");
    broken.division = "primera division";
    broken.players.push_back(makePlayer("Duplicado", "DEF", 61, 65, 27, 66, 66));
    broken.players.push_back(makePlayer("Duplicado", "N/A", 60, 64, 28, 64, 64));
    broken.players.push_back(makePlayer("Sin Arquero", "MED", 62, 68, 23, 71, 71));
    career.allTeams.push_back(broken);

    const RuntimeValidationSummary summary = validateLoadedCareerData(career, 8);
    expect(!summary.ok, "La validacion en carga debe fallar con planteles estructuralmente invalidos.");
    expect(summary.errorCount >= 2, "La validacion en carga debe contar errores de duplicados y posiciones invalidas.");
}

void testStartupValidationSummaryExposesExternalAudit() {
    const StartupValidationSummary summary = buildStartupValidationSummary(3, true);
    const DataValidationReport report = buildRosterDataValidationReport();

    expect(summary.errorCount == report.errorCount,
           "La validacion de arranque debe reflejar el mismo conteo de errores que la auditoria profunda.");
    expect(summary.warningCount == report.warningCount,
           "La validacion de arranque debe reflejar el mismo conteo de advertencias que la auditoria profunda.");
    expect(!summary.lines.empty(), "La validacion de arranque debe devolver un resumen legible.");
    if (report.errorCount > 0) {
        expect(!summary.ok, "La validacion de arranque no puede marcarse sana si la base externa tiene errores.");
    }
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

void testTransferNegotiationBuildsStructuredDeal() {
    Career career;
    career.currentSeason = 5;
    career.managerReputation = 78;

    Team buyer = makeTeam("Comprador Tactico", "primera division", 72, 4, 4, "Pressing", "Contra-presion", 2200000);
    buyer.sponsorWeekly = 140000;
    buyer.fanBase = 72;
    buyer.trainingFacilityLevel = 5;
    buyer.youthFacilityLevel = 4;
    ensureTeamIdentity(buyer);

    Team seller = makeTeam("Vendedor Historico", "primera b", 69, 3, 3, "Balanced", "Equilibrado", 480000);
    seller.fanBase = 18;
    seller.trainingFacilityLevel = 2;
    seller.youthFacilityLevel = 2;
    ensureTeamIdentity(seller);

    Player target = makePlayer("Mediapunta Premium", "MED", 77, 82, 24, 75, 77);
    target.value = 520000;
    target.releaseClause = 900000;
    target.wage = 24000;
    target.contractWeeks = 84;
    target.currentForm = 74;
    target.consistency = 70;
    target.ambition = 68;
    target.professionalism = 73;

    const NegotiationState state = runTransferNegotiation(
        career, buyer, seller, target, NegotiationProfile::Balanced, NegotiationPromise::Starter);

    expect(state.clubAccepted, "La negociacion estructurada debe cerrar el acuerdo con el club en este escenario.");
    expect(state.playerAccepted, "La negociacion estructurada debe cerrar el acuerdo con el jugador en este escenario.");
    expect(state.round >= 5, "La negociacion debe registrar varias rondas de ida y vuelta.");
    expect(state.agreedFee >= state.sellerExpectation * 90 / 100,
           "El fee acordado debe aproximarse a la expectativa del vendedor.");
    expect(state.agreedWage >= target.wage, "El salario acordado no puede empeorar el contrato actual del jugador.");
    expect(!state.roundSummaries.empty(), "La negociacion debe devolver trazabilidad de sus rondas.");
}

void testTransferShortlistRewardsScoutedNeed() {
    Career career;
    career.currentSeason = 3;
    career.managerReputation = 75;

    career.allTeams.push_back(makeTeam("Comprador Scout", "primera division", 64, 3, 3, "Balanced", "Equilibrado", 1800000));
    career.allTeams.push_back(makeTeam("Vendedor A", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 900000));
    career.allTeams.push_back(makeTeam("Vendedor B", "primera division", 69, 3, 3, "Balanced", "Equilibrado", 900000));
    Team& buyer = career.allTeams[0];
    Team& sellerA = career.allTeams[1];
    Team& sellerB = career.allTeams[2];
    buyer.players.erase(buyer.players.begin() + 5, buyer.players.begin() + 8);

    Player targetA = makePlayer("Objetivo Scouteado", "MED", 76, 84, 22, 77, 78);
    targetA.value = 420000;
    targetA.releaseClause = 780000;
    targetA.wage = 14000;
    sellerA.addPlayer(targetA);
    career.scoutingShortlist.push_back(sellerA.name + "|" + targetA.name);

    Player targetB = makePlayer("Alternativa", "MED", 75, 80, 25, 74, 75);
    targetB.value = 410000;
    targetB.releaseClause = 760000;
    targetB.wage = 14500;
    sellerB.addPlayer(targetB);

    const vector<TransferTarget> shortlist = transfer_market::buildTransferShortlist(career, buyer, 5);
    expect(!shortlist.empty(), "El mercado debe construir una shortlist para el club comprador.");
    expect(shortlist.front().onShortlist, "Un objetivo ya scouteado debe subir en la prioridad de fichaje.");
}

void testSquadPlannerFlagsUnusedSeniorSaleCandidates() {
    Team team = makeTeam("Plantel Largo", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 650000);

    Player& expendableA = team.players[12];
    expendableA.age = 31;
    expendableA.startsThisSeason = 0;
    expendableA.matchesPlayed = 0;
    expendableA.potential = expendableA.skill + 1;
    expendableA.fitness = 53;
    expendableA.wantsToLeave = true;

    Player& expendableB = team.players[13];
    expendableB.age = 29;
    expendableB.startsThisSeason = 0;
    expendableB.matchesPlayed = 1;
    expendableB.potential = expendableB.skill + 2;
    expendableB.happiness = 42;

    const SquadNeedReport report = ai_squad_planner::analyzeSquad(team);
    expect(report.unusedSeniorPlayers >= 2,
           "La IA de plantilla debe contar veteranos casi sin uso para planificar salidas.");
    expect(report.salePressure >= 1,
           "La IA de plantilla debe generar presion de venta cuando el plantel acumula piezas marginales.");
    expect(find(report.saleCandidates.begin(), report.saleCandidates.end(), expendableA.name) != report.saleCandidates.end(),
           "Un jugador veterano, poco usado y con deseo de salida debe entrar en candidatos de venta.");
}

void testSeasonTransitionAdvancesCareerWithoutUiDependencies() {
    Career career;
    career.currentSeason = 6;
    career.currentWeek = 34;
    career.managerName = "Test Manager";
    career.managerReputation = 70;

    career.allTeams.push_back(makeTeam("Campeon Capital", "primera division", 76, 3, 3, "Balanced", "Equilibrado", 900000));
    career.allTeams.push_back(makeTeam("Puerto Unido", "primera division", 73, 3, 3, "Balanced", "Equilibrado", 820000));
    career.allTeams.push_back(makeTeam("Valle FC", "primera division", 67, 2, 2, "Defensive", "Bloque bajo", 510000));
    career.allTeams.push_back(makeTeam("Mineros del Sur", "primera division", 66, 2, 2, "Defensive", "Pausar juego", 480000));

    career.allTeams.push_back(makeTeam("Ascenso Norte", "primera b", 71, 3, 3, "Balanced", "Equilibrado", 420000));
    career.allTeams.push_back(makeTeam("Ascenso Sur", "primera b", 70, 3, 3, "Balanced", "Equilibrado", 410000));
    career.allTeams.push_back(makeTeam("Primera B Central", "primera b", 64, 2, 2, "Balanced", "Equilibrado", 340000));
    career.allTeams.push_back(makeTeam("Primera B Costa", "primera b", 63, 2, 2, "Balanced", "Equilibrado", 330000));

    career.setActiveDivision("primera division");
    expect(career.activeTeams.size() == 4, "La division activa debe cargar el bloque de equipos principal.");

    career.myTeam = career.findTeamByName("Campeon Capital");
    expect(career.myTeam != nullptr, "El equipo del usuario debe existir en el fixture.");

    const vector<int> points = {68, 61, 29, 23};
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        career.activeTeams[i]->points = points[i];
        career.activeTeams[i]->goalsFor = 30 - static_cast<int>(i) * 3;
        career.activeTeams[i]->goalsAgainst = 12 + static_cast<int>(i) * 4;
        career.activeTeams[i]->wins = points[i] / 3;
    }
    career.leagueTable.sortTable();

    SeasonTransitionSummary summary = endSeason(career);

    expect(summary.champion == "Campeon Capital", "La transicion debe reconocer al campeon de la tabla.");
    expect(!summary.lines.empty(), "La transicion de temporada debe devolver un resumen legible.");
    expect(career.currentSeason == 7, "El cierre de temporada debe avanzar el ano de carrera.");
    expect(career.currentWeek == 1, "El cierre de temporada debe reiniciar la semana actual.");
    expect(!career.history.empty(), "El historial de carrera debe registrar la temporada cerrada.");
    expect(career.history.back().champion == "Campeon Capital",
           "El historial debe guardar el campeon del ano que termino.");
    expect(career.activeDivision == career.myTeam->division,
           "Tras la transicion la carrera debe seguir la division real del club usuario.");
    expect(!career.schedule.empty(), "La nueva temporada debe reconstruir su calendario.");
}

void testSeasonServiceReturnsStructuredWeekResult() {
    Career career;
    career.currentSeason = 3;
    career.currentWeek = 1;

    career.allTeams.push_back(makeTeam("Servicio Local", "primera division", 72, 4, 4, "Pressing", "Contra-presion", 650000));
    career.allTeams.push_back(makeTeam("Servicio Visita", "primera division", 69, 2, 2, "Defensive", "Bloque bajo", 520000));
    career.allTeams.push_back(makeTeam("Servicio Centro", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 500000));
    career.allTeams.push_back(makeTeam("Servicio Sur", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 470000));

    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Servicio Local");
    expect(career.myTeam != nullptr, "El fixture del SeasonService debe tener club de usuario.");
    expect(!career.schedule.empty(), "El fixture del SeasonService debe crear calendario.");

    SeasonService service;
    SeasonStepResult result = service.simulateWeek(career);

    expect(result.ok, "SeasonService debe completar la simulacion semanal.");
    expect(result.week.ok, "WeekSimulationResult debe marcarse como exitoso.");
    expect(result.week.weekBefore == 1 && result.week.weekAfter == 2,
           "SeasonService debe avanzar exactamente una semana regular.");
    expect(result.week.seasonBefore == 3 && result.week.seasonAfter == 3,
           "Una fecha normal no debe cerrar la temporada.");
    expect(!result.week.messages.empty(), "SeasonService debe devolver mensajes estructurados.");
    expect(!result.week.lastMatchAnalysis.empty(), "SeasonService debe propagar el ultimo analisis de partido.");
}

void testOpponentReportExplainsNextFixture() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 1;

    career.allTeams.push_back(makeTeam("Reporte Azul", "primera division", 71, 4, 3, "Pressing", "Contra-presion", 600000));
    career.allTeams.push_back(makeTeam("Reporte Rival", "primera division", 69, 2, 4, "Counter", "Juego directo", 520000));
    career.allTeams.push_back(makeTeam("Reporte Centro", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 500000));
    career.allTeams.push_back(makeTeam("Reporte Sur", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 470000));

    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Reporte Azul");
    expect(career.myTeam != nullptr, "El reporte rival necesita club usuario.");

    const string report = buildOpponentReport(career);
    expect(report.find("Sin informe rival disponible") == string::npos,
           "El informe rival no debe quedar vacio cuando existe una fecha activa.");
    expect(report.find("lineas") != string::npos, "El informe rival debe incluir lectura por lineas.");
    expect(report.find("alerta") != string::npos, "El informe rival debe incluir una vulnerabilidad o alerta.");
}

void testMatchAnalysisStoreProducesStructuredCareerData() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 4;
    career.allTeams.push_back(makeTeam("Analisis Local", "primera division", 72, 4, 4, "Pressing", "Contra-presion", 700000));
    career.allTeams.push_back(makeTeam("Analisis Rival", "primera division", 68, 2, 3, "Counter", "Juego directo", 620000));
    career.myTeam = &career.allTeams[0];

    const MatchResult result = match_engine::simulate(career.allTeams[0], career.allTeams[1], true, false).result;
    career_match_analysis::storeMatchAnalysis(career, career.allTeams[0], career.allTeams[1], result, false);

    expect(!career.lastMatchAnalysis.empty(), "El servicio de analisis debe guardar un resumen del ultimo partido.");
    expect(!career.lastMatchReportLines.empty(), "El servicio de analisis debe guardar lineas estructuradas del reporte.");
    expect(!career.lastMatchEvents.empty(), "El servicio de analisis debe guardar eventos del partido.");
    expect(!career.lastMatchPlayerOfTheMatch.empty(), "El servicio de analisis debe conservar la figura del partido.");

    const string detail = career_match_analysis::buildLastMatchInsightText(career, 3, 4);
    expect(detail.find("MatchReport") != string::npos, "La vista detallada debe incluir el bloque MatchReport.");
    expect(detail.find("MatchTimeline") != string::npos, "La vista detallada debe incluir el bloque MatchTimeline.");
}

void testMatchCenterServiceBuildsStructuredView() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 7;
    career.allTeams.push_back(makeTeam("Centro Local", "primera division", 73, 4, 4, "Pressing", "Contra-presion", 760000));
    career.allTeams.push_back(makeTeam("Centro Rival", "primera division", 69, 2, 3, "Counter", "Juego directo", 630000));
    career.myTeam = &career.allTeams[0];

    const MatchResult result = match_engine::simulate(career.allTeams[0], career.allTeams[1], true, false).result;
    career_match_analysis::storeMatchAnalysis(career, career.allTeams[0], career.allTeams[1], result, false);

    const MatchCenterView center = match_center_service::buildLastMatchCenter(career, 3, 4);
    expect(center.available, "El match center debe marcarse disponible tras almacenar un partido.");
    expect(!center.scoreboard.empty(), "El match center debe construir un marcador legible.");
    expect(center.metrics.size() >= 4, "El match center debe exponer metricas comparables.");
    expect(!center.phaseLines.empty(), "El match center debe conservar lectura por fases.");
    expect(!center.eventLines.empty(), "El match center debe conservar una timeline resumida.");
    expect(!career.lastMatchCenter.opponentName.empty(), "El snapshot persistente del ultimo partido debe llenarse.");
}

void testDressingRoomServiceFlagsPromiseAndFatigueRisk() {
    Career career;
    career.currentSeason = 4;
    career.currentWeek = 8;
    career.allTeams.push_back(makeTeam("Vestuario Unido", "primera division", 70, 4, 4, "Pressing", "Contra-presion", 720000));
    career.myTeam = &career.allTeams[0];

    Player& starter = career.myTeam->players[1];
    starter.promisedRole = "Titular";
    starter.startsThisSeason = 1;
    starter.happiness = 44;
    starter.fitness = 53;
    starter.contractWeeks = 10;

    Player& rotation = career.myTeam->players[2];
    rotation.promisedRole = "Rotacion";
    rotation.startsThisSeason = 0;
    rotation.happiness = 42;
    rotation.fitness = 56;

    const DressingRoomSnapshot snapshotBefore =
        dressing_room_service::buildSnapshot(*career.myTeam, career.currentWeek);
    expect(snapshotBefore.promiseRiskCount >= 2, "El snapshot debe detectar promesas en riesgo.");
    expect(snapshotBefore.fatigueRiskCount >= 2, "El snapshot debe detectar jugadores fundidos.");

    const DressingRoomSnapshot snapshotAfter = dressing_room_service::applyWeeklyUpdate(career, 0);
    expect(snapshotAfter.lowMoraleCount >= 2, "La actualizacion semanal debe reflejar mal clima.");
    expect(!snapshotAfter.summary.empty(), "La actualizacion semanal debe devolver un resumen de vestuario.");
    expect(career.newsFeed.size() > 0, "Una semana conflictiva debe dejar noticia de vestuario.");
}

void testSaveLoadRoundTripPreservesCareerState() {
    const string savePath = "saves/test_roundtrip_save.txt";

    Career original;
    original.currentSeason = 8;
    original.currentWeek = 5;
    original.managerName = "Arquitecto|Senior";
    original.managerReputation = 77;
    original.boardConfidence = 66;
    original.boardExpectedFinish = 3;
    original.boardBudgetTarget = 250000;
    original.boardYouthTarget = 2;
    original.boardWarningWeeks = 1;
    original.boardMonthlyObjective = "Sumar; al menos ^6 puntos | en 4 semanas";
    original.boardMonthlyTarget = 6;
    original.boardMonthlyProgress = 4;
    original.boardMonthlyDeadlineWeek = 8;
    original.lastMatchAnalysis = "Control total | del mediocampo; con ajuste^final";
    original.lastMatchReportLines = {"Claves| se domino el mediocampo", "Fatiga; el rival llego roto", "Disciplina^ sin expulsiones"};
    original.lastMatchEvents = {"12' Gol | de prueba", "64' Ajuste; tactico", "88' Atajada ^ clave"};
    original.lastMatchPlayerOfTheMatch = "Jugador|Persistente";
    original.lastMatchCenter.competitionLabel = "Liga";
    original.lastMatchCenter.opponentName = "Club Destino";
    original.lastMatchCenter.venueLabel = "Local";
    original.lastMatchCenter.myGoals = 2;
    original.lastMatchCenter.oppGoals = 1;
    original.lastMatchCenter.myShots = 11;
    original.lastMatchCenter.oppShots = 8;
    original.lastMatchCenter.myShotsOnTarget = 5;
    original.lastMatchCenter.oppShotsOnTarget = 3;
    original.lastMatchCenter.myPossession = 56;
    original.lastMatchCenter.oppPossession = 44;
    original.lastMatchCenter.myCorners = 6;
    original.lastMatchCenter.oppCorners = 2;
    original.lastMatchCenter.mySubstitutions = 4;
    original.lastMatchCenter.oppSubstitutions = 5;
    original.lastMatchCenter.myExpectedGoalsTenths = 17;
    original.lastMatchCenter.oppExpectedGoalsTenths = 9;
    original.lastMatchCenter.weather = "Despejado; viento";
    original.lastMatchCenter.dominanceSummary = "El local controlo | la zona media.";
    original.lastMatchCenter.tacticalSummary = "La presion alta sostuvo recuperaciones ^altas.";
    original.lastMatchCenter.fatigueSummary = "El rival llego; fundido al cierre.";
    original.lastMatchCenter.postMatchImpact = "Moral +3 | / -2";
    original.lastMatchCenter.phaseSummaries = {"1-15: domina | Club Persistencia", "16-30: domina ^ Club Persistencia"};
    original.newsFeed.push_back("T8-F5: Noticia| de prueba;");
    original.scoutInbox.push_back("Informe^ de ojeo");
    original.scoutingShortlist.push_back("Promesa| del norte; central");
    original.history.push_back({7, "primera division", "Club Persistencia", 2, "Club Persistencia", "Ascenso| Norte", "Descenso; Sur", "Nota ^ historica"});
    original.activePromises.push_back({"Proyecto Norte", "Minutos", "Proyecto", 5, 10, 2, false, false});
    original.historicalRecords.push_back({"Primera Division - Mas puntos", "Club Persistencia", "Club Persistencia", 7, 71, "Puntos en una temporada"});
    original.pendingTransfers.push_back({"Jugador| Pendiente", "Club Persistencia", "Club Destino", 9, 0, 120000, 9000, 104, false, false, "Titular^"});

    original.allTeams.push_back(makeTeam("Club Persistencia", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    original.allTeams.push_back(makeTeam("Club Destino", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 650000));
    original.setActiveDivision("primera division");
    original.myTeam = original.findTeamByName("Club Persistencia");
    expect(original.myTeam != nullptr, "El fixture de persistencia debe tener club de usuario.");
    original.myTeam->players[0].moraleMomentum = 7;
    original.myTeam->players[0].fatigueLoad = 31;
    original.myTeam->players[0].unhappinessWeeks = 2;
    original.myTeam->players[0].promisedPosition = "DEF";
    original.myTeam->players[0].socialGroup = "Lideres";
    original.saveFile = savePath;

    expect(original.saveCareer(), "El guardado roundtrip debe completarse.");

    Career loaded;
    loaded.saveFile = savePath;
    expect(loaded.loadCareer(), "La carga roundtrip debe completarse.");
    expect(loaded.managerName == original.managerName, "La carga debe preservar el nombre del manager.");
    expect(loaded.managerReputation == original.managerReputation, "La carga debe preservar la reputacion del manager.");
    expect(loaded.boardConfidence == original.boardConfidence, "La carga debe preservar confianza de directiva.");
    expect(loaded.lastMatchAnalysis == original.lastMatchAnalysis, "La carga debe preservar el ultimo analisis de partido.");
    expect(loaded.lastMatchReportLines.size() == original.lastMatchReportLines.size(),
           "La carga debe preservar el reporte estructurado del ultimo partido.");
    expect(loaded.lastMatchEvents.size() == original.lastMatchEvents.size(),
           "La carga debe preservar la timeline del ultimo partido.");
    expect(loaded.lastMatchPlayerOfTheMatch == original.lastMatchPlayerOfTheMatch,
           "La carga debe preservar la figura del ultimo partido.");
    expect(loaded.lastMatchCenter.opponentName == original.lastMatchCenter.opponentName,
           "La carga debe preservar el snapshot del match center.");
    expect(loaded.lastMatchCenter.phaseSummaries.size() == original.lastMatchCenter.phaseSummaries.size(),
           "La carga debe preservar las fases resumidas del match center.");
    expect(loaded.history.size() == 1 && loaded.history.front().champion == "Club Persistencia",
           "La carga debe preservar historial de temporada.");
    expect(!loaded.activePromises.empty() && loaded.activePromises.front().category == "Minutos",
           "La carga debe preservar promesas activas.");
    expect(!loaded.historicalRecords.empty() && loaded.historicalRecords.front().value == 71,
           "La carga debe preservar records historicos.");
    expect(loaded.myTeam != nullptr && loaded.myTeam->name == "Club Persistencia",
           "La carga debe restaurar el club controlado.");
    expect(loaded.myTeam->players[0].moraleMomentum == 7 &&
               loaded.myTeam->players[0].fatigueLoad == 31 &&
               loaded.myTeam->players[0].promisedPosition == "DEF",
           "La carga debe preservar momento, carga y promesa de posicion del jugador.");
    expect(!loaded.pendingTransfers.empty() && loaded.pendingTransfers.front().toTeam == "Club Destino",
           "La carga debe preservar fichajes pendientes.");

    std::remove(savePath.c_str());
}

void testLoadTeamFromDirectoryFallsBackAndResolvesRawPositions() {
    const string folderPath = "saves/test_loader_team";
    const string playersPath = joinPath(folderPath, "players.txt");

    expect(ensureDirectory(folderPath), "No se pudo crear la carpeta temporal del loader.");
    ofstream file(playersPath);
    expect(file.is_open(), "No se pudo crear el players.txt temporal.");
    file
        << "- Uno | DEF (Uno Goalkeeper) | Edad: 25 | Valor: 120k\n"
        << "- Dos | N/A (Dos Goalkeeper) | Edad: 22 | Valor: 90k\n"
        << "- Tres | DEL (Tres Central Midfield) | Edad: 24 | Valor: 180k\n"
        << "- Cuatro | DEF (Cuatro Central Midfield) | Edad: 21 | Valor: 150k\n"
        << "- Cinco | MED (Cinco Attacking Midfield) | Edad: 20 | Valor: 140k\n"
        << "- Seis | DEL (Seis Centre-Forward) | Edad: 26 | Valor: 200k\n";
    file.close();

    Team loaded("Temporal FC");
    loaded.division = "primera b";
    expect(loadTeamFromFile(folderPath, loaded), "El loader debe caer a players.txt cuando no existe CSV.");
    expect(static_cast<int>(loaded.players.size()) >= 18, "El loader debe completar un plantel jugable.");

    Team* teamPtr = &loaded;
    int goalkeepers = 0;
    int midfielders = 0;
    for (const auto& player : teamPtr->players) {
        if (normalizePosition(player.position) == "ARQ") goalkeepers++;
        if (normalizePosition(player.position) == "MED") midfielders++;
    }
    expect(goalkeepers >= 2, "El loader debe garantizar cobertura minima de arqueros.");
    expect(midfielders >= 3, "El loader debe garantizar cobertura minima de mediocampo.");

    auto findPlayer = [&](const string& name) -> const Player* {
        for (const auto& player : teamPtr->players) {
            if (player.name == name) return &player;
        }
        return nullptr;
    };

    const Player* one = findPlayer("Uno");
    const Player* three = findPlayer("Tres");
    expect(one && one->position == "ARQ", "El loader debe resolver Goalkeeper desde la posicion cruda.");
    expect(three && three->position == "MED", "El loader debe priorizar la posicion cruda cuando la principal es inconsistente.");

    std::remove(playersPath.c_str());
}

void testSimulateMatchAppliesPostProcessState() {
    Team home = makeTeam("Aplicacion Local", "primera division", 71, 4, 4, "Pressing", "Contra-presion", 700000);
    Team away = makeTeam("Aplicacion Visita", "primera division", 68, 2, 3, "Balanced", "Equilibrado", 620000);

    const MatchResult result = simulateMatch(home, away, true, false);

    expect(home.goalsFor == result.homeGoals && home.goalsAgainst == result.awayGoals,
           "La aplicacion postpartido debe actualizar goles del local.");
    expect(away.goalsFor == result.awayGoals && away.goalsAgainst == result.homeGoals,
           "La aplicacion postpartido debe actualizar goles del visitante.");
    expect(away.awayGoals == result.awayGoals,
           "La aplicacion postpartido debe contabilizar goles de visita.");
    expect(home.points + away.points >= 2 && home.points + away.points <= 3,
           "La aplicacion postpartido debe repartir puntos validos.");

    int homePlayed = 0;
    for (const Player& player : home.players) {
        if (player.matchesPlayed > 0) homePlayed++;
    }
    int awayPlayed = 0;
    for (const Player& player : away.players) {
        if (player.matchesPlayed > 0) awayPlayed++;
    }
    expect(homePlayed >= 11 && awayPlayed >= 11,
           "La aplicacion postpartido debe registrar minutos para los participantes.");
}

void testWorldStateSeedsPromisesAndRecords() {
    Career career;
    career.currentSeason = 3;
    career.currentWeek = 2;
    career.boardExpectedFinish = 2;

    career.allTeams.push_back(makeTeam("Pulso Club", "primera division", 72, 4, 4, "Pressing", "Contra-presion", 700000));
    career.allTeams.push_back(makeTeam("Pulso Rival", "primera division", 68, 2, 3, "Balanced", "Equilibrado", 620000));
    career.allTeams.push_back(makeTeam("Pulso Norte", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 560000));
    career.allTeams.push_back(makeTeam("Pulso Sur", "primera division", 65, 2, 2, "Defensive", "Pausar juego", 550000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Pulso Club");
    expect(career.myTeam != nullptr, "La prueba de mundo necesita club usuario.");

    world_state_service::seedSeasonPromises(career);
    expect(career.activePromises.size() >= 2,
           "El estado del mundo debe sembrar promesas activas de competicion y gestion de plantilla.");

    career.leagueTable.sortTable();
    career.myTeam->points = 73;
    career.myTeam->goalsFor = 44;
    career.myTeam->goalsAgainst = 16;
    career.myTeam->players[8].goals = 17;
    career.leagueTable.sortTable();

    const int updates = world_state_service::updateSeasonRecords(career, career.leagueTable);
    expect(updates >= 1, "El estado del mundo debe registrar al menos un record historico cuando se supera una marca.");
    expect(!career.historicalRecords.empty(), "Los records historicos deben persistirse en la carrera.");
}

void testWeeklyWorldStateResolvesMinutesPromise() {
    Career career;
    career.currentSeason = 4;
    career.currentWeek = 8;

    career.allTeams.push_back(makeTeam("Promesa Activa", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Rival Uno", "primera division", 67, 2, 2, "Defensive", "Bloque bajo", 620000));
    career.allTeams.push_back(makeTeam("Rival Dos", "primera division", 66, 3, 3, "Balanced", "Equilibrado", 610000));
    career.allTeams.push_back(makeTeam("Rival Tres", "primera division", 65, 2, 2, "Defensive", "Pausar juego", 600000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Promesa Activa");
    expect(career.myTeam != nullptr, "La prueba de promesas necesita club usuario.");

    Player& youngster = career.myTeam->players[0];
    youngster.promisedRole = "Proyecto";
    youngster.startsThisSeason = 2;
    career.activePromises.push_back({youngster.name, "Minutos", "Proyecto", 4, 8, 2, false, false});

    const WorldPulseSummary summary = world_state_service::processWeeklyWorldState(career);
    expect(summary.resolvedPromises >= 1, "Una promesa de minutos cumplida debe resolverse en el pulso semanal.");
    expect(career.activePromises.empty(), "Las promesas resueltas deben salir del backlog activo.");
    expect(youngster.happiness >= 64, "Cumplir una promesa debe mejorar el estado del jugador.");
}

}  // namespace

int main() {
    const vector<pair<string, void (*)()>> tests = {
        {"validation_suite", testValidationSuiteReflectsRosterAudit},
        {"match_engine_structure", testMatchSimulationProducesStructuredPhases},
        {"tactical_fatigue", testHighPressRaisesPhaseFatigue},
        {"low_block_chance_quality", testLowBlockSuppressesChanceQuality},
        {"competition_rules_csv", testCompetitionRulesLoadFromCsv},
        {"runtime_load_validation", testRuntimeValidationFlagsBrokenLoadedSquad},
        {"startup_data_validation", testStartupValidationSummaryExposesExternalAudit},
        {"competition_group_table", testCompetitionGroupTableScopesActiveGroup},
        {"transfer_affordability", testTransferEvaluationPenalizesUnaffordableDeals},
        {"transfer_shortlist", testTransferShortlistRewardsScoutedNeed},
        {"squad_sale_candidates", testSquadPlannerFlagsUnusedSeniorSaleCandidates},
        {"transfer_negotiation", testTransferNegotiationBuildsStructuredDeal},
        {"season_transition", testSeasonTransitionAdvancesCareerWithoutUiDependencies},
        {"season_service", testSeasonServiceReturnsStructuredWeekResult},
        {"opponent_report", testOpponentReportExplainsNextFixture},
        {"match_analysis_store", testMatchAnalysisStoreProducesStructuredCareerData},
        {"match_center_service", testMatchCenterServiceBuildsStructuredView},
        {"dressing_room_service", testDressingRoomServiceFlagsPromiseAndFatigueRisk},
        {"world_state_seed", testWorldStateSeedsPromisesAndRecords},
        {"world_state_promises", testWeeklyWorldStateResolvesMinutesPromise},
        {"simulate_match_state", testSimulateMatchAppliesPostProcessState},
        {"save_load_roundtrip", testSaveLoadRoundTripPreservesCareerState},
        {"loader_fallback", testLoadTeamFromDirectoryFallsBackAndResolvesRawPositions},
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
