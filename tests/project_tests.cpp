#include "ai/ai_squad_planner.h"
#include "ai/ai_transfer_manager.h"
#include "career/analytics_service.h"
#include "career/app_services.h"
#include "career/career_reports.h"
#include "career/inbox_service.h"
#include "career/manager_advice.h"
#include "career/transfer_briefing.h"
#include "career/dressing_room_service.h"
#include "career/medical_service.h"
#include "career/staff_service.h"
#include "career/match_analysis_store.h"
#include "career/match_center_service.h"
#include "career/career_support.h"
#include "career/season_service.h"
#include "career/season_transition.h"
#include "career/world_state_service.h"
#include "competition/competition.h"
#include "competition/league_registry.h"
#include "development/monthly_development.h"
#include "development/training_impact_system.h"
#include "engine/models.h"
#include "engine/game_settings.h"
#include "io/save_serialization.h"
#include "io/io.h"
#include "simulation/match_context.h"
#include "simulation/match_engine_internal.h"
#include "simulation/match_engine.h"
#include "simulation/match_event_generator.h"
#include "simulation/match_phase.h"
#include "simulation/player_condition.h"
#include "simulation/simulation.h"
#include "transfers/negotiation_system.h"
#include "transfers/transfer_market.h"
#include "utils/utils.h"
#include "validators/validators.h"

#include <algorithm>
#include <cstddef>
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

string replaceAllCopy(string text, const string& needle, const string& replacement) {
    if (needle.empty()) return text;
    size_t pos = 0;
    while ((pos = text.find(needle, pos)) != string::npos) {
        text.replace(pos, needle.size(), replacement);
        pos += replacement.size();
    }
    return text;
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
    } else {
        expect(summary.ok, "La suite debe quedar sana cuando no existen errores de datos ni fallas logicas.");
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

    setRandomSeed(20260320);
    const MatchSetup lowBlockSetup = match_context::buildMatchSetup(home, lowBlock, false, false);
    setRandomSeed(20260320);
    const MatchSetup openSetup = match_context::buildMatchSetup(home, openGame, false, false);

    setRandomSeed(424242);
    const MatchPhaseEvaluation lowBlockEval =
        match_phase::evaluatePhase(lowBlockSetup, home, lowBlock, lowBlockSetup.home, lowBlockSetup.away, 1, 16, 30);
    setRandomSeed(424242);
    const MatchPhaseEvaluation openEval =
        match_phase::evaluatePhase(openSetup, home, openGame, openSetup.home, openSetup.away, 1, 16, 30);
    resetRandomSeed();

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

void testDressingRoomSnapshotTracksSocialTension() {
    Career career;
    career.currentSeason = 5;
    career.currentWeek = 9;
    career.allTeams.push_back(makeTeam("Grupo Quebrado", "primera division", 69, 3, 3, "Balanced", "Equilibrado", 710000));
    career.myTeam = &career.allTeams[0];

    for (int idx : {1, 2, 3}) {
        Player& player = career.myTeam->players[static_cast<size_t>(idx)];
        player.happiness = 39;
        player.startsThisSeason = 0;
        player.unhappinessWeeks = 4;
        player.wantsToLeave = true;
    }
    career.myTeam->players[0].leadership = 80;
    career.myTeam->players[0].happiness = 67;

    const DressingRoomSnapshot snapshot = dressing_room_service::buildSnapshot(*career.myTeam, career.currentWeek);
    expect(snapshot.socialTension >= 4, "El snapshot debe medir tension social cuando se acumulan focos de conflicto.");
    expect(snapshot.coreGroupSize >= 2, "El vestuario debe identificar un grupo dominante.");
    expect(find(snapshot.groups.begin(), snapshot.groups.end(), string("Frustrados: 3")) != snapshot.groups.end(),
           "Los jugadores conflictivos deben agruparse como foco social visible.");
}

void testScoutingConfidenceReflectsStaffQuality() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 3;
    career.allTeams.push_back(makeTeam("Club Scout", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 1500000));
    career.allTeams.push_back(makeTeam("Club Fuente", "primera division", 72, 3, 3, "Balanced", "Equilibrado", 900000));
    career.allTeams.push_back(makeTeam("Club Apoyo", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 850000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Club Scout");
    expect(career.myTeam != nullptr, "La prueba de scouting necesita club usuario.");

    career.myTeam->scoutingChief = 45;
    career.myTeam->budget = 1500000;
    const ScoutingSessionResult low = runScoutingSessionService(career, "Todas", "MED");
    expect(low.service.ok && !low.candidates.empty(), "El scouting debe devolver candidatos en el escenario base.");
    const int lowConfidence = low.candidates.front().confidence;
    expect(low.candidates.front().salaryExpectation > 0,
           "El informe debe exponer una expectativa salarial utilizable.");
    expect(!low.candidates.front().riskLabel.empty(), "El informe debe etiquetar el riesgo del objetivo.");

    career.myTeam->scoutingChief = 88;
    career.myTeam->budget = 1500000;
    const ScoutingSessionResult high = runScoutingSessionService(career, "Todas", "MED");
    expect(high.service.ok && !high.candidates.empty(), "El scouting de alto nivel debe devolver candidatos.");
    expect(high.candidates.front().confidence > lowConfidence,
           "Un mejor jefe de scouting debe elevar la confianza del informe.");
}

void testSeasonTransitionResolvesCarryoverPromises() {
    Career career;
    career.currentSeason = 6;
    career.currentWeek = 34;
    career.managerName = "Manager Promesas";
    career.managerReputation = 66;

    career.allTeams.push_back(makeTeam("Promesa Capital", "primera division", 75, 3, 3, "Balanced", "Equilibrado", 900000));
    career.allTeams.push_back(makeTeam("Promesa Norte", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 760000));
    career.allTeams.push_back(makeTeam("Promesa Sur", "primera division", 67, 2, 2, "Defensive", "Bloque bajo", 580000));
    career.allTeams.push_back(makeTeam("Promesa Costa", "primera division", 66, 2, 2, "Defensive", "Pausar juego", 560000));
    career.allTeams.push_back(makeTeam("Ascenso A", "primera b", 69, 3, 3, "Balanced", "Equilibrado", 420000));
    career.allTeams.push_back(makeTeam("Ascenso B", "primera b", 68, 3, 3, "Balanced", "Equilibrado", 400000));
    career.allTeams.push_back(makeTeam("Ascenso C", "primera b", 64, 2, 2, "Balanced", "Equilibrado", 330000));
    career.allTeams.push_back(makeTeam("Ascenso D", "primera b", 63, 2, 2, "Balanced", "Equilibrado", 320000));

    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Promesa Capital");
    expect(career.myTeam != nullptr, "La prueba de transicion necesita club usuario.");

    career.myTeam->players[1].name = "Promesa Tardia";
    career.myTeam->players[1].promisedRole = "Titular";
    career.myTeam->players[1].startsThisSeason = 1;
    career.activePromises.push_back({"Promesa Tardia", "Minutos", "Titular", 30, 40, 1, false, false});

    const vector<int> points = {70, 61, 29, 24};
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        career.activeTeams[i]->points = points[i];
        career.activeTeams[i]->goalsFor = 31 - static_cast<int>(i) * 3;
        career.activeTeams[i]->goalsAgainst = 12 + static_cast<int>(i) * 4;
        career.activeTeams[i]->wins = points[i] / 3;
    }
    career.leagueTable.sortTable();

    SeasonTransitionSummary summary = endSeason(career);
    const string lines = joinLines(summary.lines);
    expect(lines.find("Promesas cerradas al final del curso") != string::npos,
           "La transicion debe cerrar promesas pendientes antes de resetear la temporada.");
    expect(lines.find("Promesa rota:") != string::npos || lines.find("Promesa cerrada:") != string::npos,
           "La transicion debe explicar como se resolvio la promesa arrastrada.");
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
    original.managerInbox.push_back("[Resumen] T8-F5: Bandeja| de manager");
    original.scoutingAssignments.push_back({"Sur", "DEF", "Urgente", 2, 61});
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
    original.myTeam->players[0].roleDuty = "Ataque";
    original.myTeam->players[0].individualInstruction = "Marcar fuerte";
    original.myTeam->headCoachName = "DT Persistente";
    original.myTeam->headCoachStyle = "Presion";
    original.myTeam->headCoachTenureWeeks = 28;
    original.myTeam->jobSecurity = 47;
    original.myTeam->transferPolicy = "Cantera y valor futuro";
    original.myTeam->scoutingRegions = {"Metropolitana", "Sur", "Internacional"};
    original.myTeam->assistantCoachName = "Ayudante Persistente";
    original.myTeam->fitnessCoachName = "PF Persistente";
    original.myTeam->scoutingChiefName = "Scout Persistente";
    original.myTeam->youthCoachName = "Juveniles Persistente";
    original.myTeam->medicalChiefName = "Medico Persistente";
    original.myTeam->goalkeepingCoachName = "Arqueros Persistente";
    original.myTeam->performanceAnalystName = "Analista Persistente";
    expect(original.myTeam->headCoachTenureWeeks == 28 &&
               original.myTeam->assistantCoachName == "Ayudante Persistente",
           "El fixture debe sembrar tenure y staff nominal antes del guardado.");
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
    expect(!loaded.managerInbox.empty() && loaded.managerInbox.front().find("Bandeja") != string::npos,
           "La carga debe preservar la bandeja del manager.");
    expect(!loaded.activePromises.empty() && loaded.activePromises.front().category == "Minutos",
           "La carga debe preservar promesas activas.");
    expect(!loaded.historicalRecords.empty() && loaded.historicalRecords.front().value == 71,
           "La carga debe preservar records historicos.");
    expect(loaded.myTeam != nullptr && loaded.myTeam->name == "Club Persistencia",
           "La carga debe restaurar el club controlado.");
    expect(loaded.myTeam->players[0].moraleMomentum == 7 &&
               loaded.myTeam->players[0].fatigueLoad == 31 &&
               loaded.myTeam->players[0].promisedPosition == "DEF" &&
               loaded.myTeam->players[0].roleDuty == "Ataque" &&
               loaded.myTeam->players[0].individualInstruction == "Marcar fuerte",
           "La carga debe preservar momento, carga, duty, instruccion y promesa de posicion del jugador.");
    expect(loaded.myTeam->headCoachName == "DT Persistente" &&
               loaded.myTeam->transferPolicy == "Cantera y valor futuro" &&
               loaded.myTeam->scoutingRegions.size() == 3 &&
               loaded.myTeam->headCoachTenureWeeks == 28,
           "La carga debe preservar metadata de entrenador, politica, red de scouting y antiguedad del club.");
    expect(loaded.myTeam->assistantCoachName == "Ayudante Persistente" &&
               loaded.myTeam->performanceAnalystName == "Analista Persistente",
           "La carga debe preservar el staff con nombre propio.");
    expect(!loaded.pendingTransfers.empty() && loaded.pendingTransfers.front().toTeam == "Club Destino",
           "La carga debe preservar fichajes pendientes.");
    expect(!loaded.scoutingAssignments.empty() && loaded.scoutingAssignments.front().region == "Sur",
           "La carga debe preservar asignaciones activas de scouting.");

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


void testWeeklyTrainingScheduleAdaptsToCongestion() {
    Team team = makeTeam("Microciclo FC", "primera division", 70, 4, 4, "Pressing", "Juego directo", 700000);
    team.trainingFocus = "Ataque";

    const auto regular = development::buildWeeklyTrainingSchedule(team, false);
    const auto congested = development::buildWeeklyTrainingSchedule(team, true);
    expect(regular.size() == 6 && congested.size() == 6,
           "El microciclo semanal debe construir seis sesiones visibles.");

    int regularLoad = 0;
    for (const auto& session : regular) regularLoad += session.load;
    int congestedLoad = 0;
    for (const auto& session : congested) congestedLoad += session.load;
    expect(congestedLoad < regularLoad,
           "La semana congestionada debe bajar la carga total del entrenamiento.");
    expect(congested.front().focus == "Recuperacion",
           "La semana congestionada debe abrir con recuperacion.");
}

void testTeamMeetingImprovesMorale() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 6;
    career.allTeams.push_back(makeTeam("Union Manager", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Rival Uno", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 620000));
    career.allTeams.push_back(makeTeam("Rival Dos", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 610000));
    career.allTeams.push_back(makeTeam("Rival Tres", "primera division", 65, 2, 2, "Defensive", "Pausar juego", 600000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Union Manager");
    expect(career.myTeam != nullptr, "La prueba de reunion necesita club usuario.");

    career.myTeam->morale = 44;
    career.myTeam->players[0].happiness = 32;
    career.myTeam->players[0].wantsToLeave = true;
    const int previousMorale = career.myTeam->morale;
    const int previousHappiness = career.myTeam->players[0].happiness;

    const ServiceResult result = holdTeamMeetingService(career);
    expect(result.ok, "La reunion de plantel debe completar correctamente.");
    expect(career.myTeam->morale > previousMorale,
           "La reunion debe mejorar la moral colectiva.");
    expect(career.myTeam->players[0].happiness > previousHappiness,
           "La reunion debe mejorar el estado del jugador mas afectado.");
}

void testPlayerInstructionServiceCyclesInstruction() {
    Career career;
    career.currentSeason = 1;
    career.currentWeek = 2;
    career.allTeams.push_back(makeTeam("Instruccion FC", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Rival A", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 620000));
    career.allTeams.push_back(makeTeam("Rival B", "primera division", 66, 2, 2, "Defensive", "Pausar juego", 610000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Instruccion FC");
    expect(career.myTeam != nullptr, "La prueba de instrucciones necesita club usuario.");

    Player& player = career.myTeam->players[8];
    const string before = player.individualInstruction;
    const ServiceResult result = cyclePlayerInstructionService(career, player.name);
    expect(result.ok, "La instruccion individual debe poder ciclarse desde servicio.");
    expect(player.individualInstruction != before, "La instruccion individual debe cambiar al menos un paso.");
}

void testInboxDecisionPrioritizesRecovery() {
    Career career;
    career.currentSeason = 1;
    career.currentWeek = 4;
    career.allTeams.push_back(makeTeam("Inbox Medico", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Rival A", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 620000));
    career.allTeams.push_back(makeTeam("Rival B", "primera division", 66, 2, 2, "Defensive", "Pausar juego", 610000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Inbox Medico");
    expect(career.myTeam != nullptr, "La prueba de inbox medico necesita club usuario.");

    Player& player = career.myTeam->players[0];
    player.fitness = 48;
    player.fatigueLoad = 78;
    career.addInboxItem("Lesion y fatiga acumulada en el lateral titular.", "Medical");

    const ServiceResult result = resolveInboxDecisionService(career);
    expect(result.ok, "El inbox debe poder disparar una decision medica automatizada.");
    expect(career.myTeam->trainingFocus == "Recuperacion", "La decision medica debe orientar la semana a recuperacion.");
    expect(player.individualInstruction == "Descanso medico", "El jugador en riesgo debe recibir proteccion individual.");
}

void testStaffReviewImprovesWeakestArea() {
    Career career;
    career.currentSeason = 1;
    career.currentWeek = 3;
    career.allTeams.push_back(makeTeam("Staff Review", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 1200000));
    career.allTeams.push_back(makeTeam("Rival A", "primera division", 66, 2, 2, "Defensive", "Bloque bajo", 620000));
    career.allTeams.push_back(makeTeam("Rival B", "primera division", 66, 2, 2, "Defensive", "Pausar juego", 610000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Staff Review");
    expect(career.myTeam != nullptr, "La prueba de staff necesita club usuario.");

    career.myTeam->medicalTeam = 42;
    career.myTeam->medicalChiefName.clear();
    const ServiceResult result = reviewStaffStructureService(career);
    expect(result.ok, "La revision de staff debe poder reforzar el area mas debil.");
    expect(career.myTeam->medicalTeam > 42, "La revision de staff debe subir el nivel del area mas debil.");
}

void testScoutingAssignmentShowsInReport() {
    Career career;
    career.currentSeason = 1;
    career.currentWeek = 2;
    career.allTeams.push_back(makeTeam("Asignacion Scout", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Rival A", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 620000));
    career.allTeams.push_back(makeTeam("Rival B", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 610000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Asignacion Scout");
    expect(career.myTeam != nullptr, "La prueba de asignacion necesita club usuario.");

    const ServiceResult result = createScoutingAssignmentService(career, "Sur", "DEF", 3);
    expect(result.ok, "La asignacion de scouting debe crearse correctamente.");
    expect(career.scoutingAssignments.size() == 1, "La carrera debe conservar la asignacion activa.");
    const CareerReport report = buildScoutingReport(career);
    const string dump = formatCareerReport(report);
    expect(dump.find("Sur") != string::npos && dump.find("DEF") != string::npos,
           "El informe de scouting debe mostrar las asignaciones activas.");
}

void testNegotiationTracksAgentCosts() {
    Career career;
    Team team = makeTeam("Club Grande", "primera division", 74, 4, 4, "Pressing", "Contra-presion", 1500000);
    Player player = team.players.front();
    player.contractWeeks = 40;
    player.value = max(player.value, 200000LL);
    player.releaseClause = max(player.releaseClause, 450000LL);

    const NegotiationState state = runRenewalNegotiation(career,
                                                         team,
                                                         player,
                                                         NegotiationProfile::Balanced,
                                                         NegotiationPromise::Starter,
                                                         12);
    expect(state.playerAccepted,
           "La renovacion de prueba debe cerrarse para validar el paquete contractual.");
    expect(state.agreedAgentFee > 0 && state.agreedLoyaltyBonus > 0,
           "La negociacion debe capturar fee de agente y bonus de fidelidad.");
    expect(state.agreedAppearanceBonus > 0,
           "La negociacion debe incluir bonus por partido.");
}

void testRoleDutyShapesMatchSnapshot() {
    Player player = makePlayer("Duty Test", "DEF", 68, 74, 24, 72, 72);
    player.role = "Carrilero";

    int attackAttack = player.attack;
    int defenseAttack = player.defense;
    player.roleDuty = "Ataque";
    match_internal::applyRoleModifier(player, attackAttack, defenseAttack);

    int attackDefend = player.attack;
    int defenseDefend = player.defense;
    player.roleDuty = "Defensa";
    match_internal::applyRoleModifier(player, attackDefend, defenseDefend);

    expect(attackAttack > attackDefend,
           "El duty ofensivo debe subir mas el peso ofensivo del rol respecto al duty defensivo.");
    expect(defenseDefend > defenseAttack,
           "El duty defensivo debe subir mas el peso defensivo del rol respecto al duty ofensivo.");
}

void testTeamIdentitySeedsWorldMetadata() {
    Team team = makeTeam("Metadata FC", "primera division", 69, 3, 3, "Pressing", "Contra-presion", 700000);
    team.headCoachName.clear();
    team.transferPolicy.clear();
    team.scoutingRegions.clear();
    team.goalkeepingCoach = 0;
    team.performanceAnalyst = 0;

    ensureTeamIdentity(team);

    expect(!team.headCoachName.empty(), "La identidad del club debe sembrar un entrenador principal.");
    expect(!team.transferPolicy.empty(), "La identidad del club debe sembrar una politica de mercado.");
    expect(!team.scoutingRegions.empty(), "La identidad del club debe sembrar una red minima de scouting.");
    expect(team.goalkeepingCoach > 0 && team.performanceAnalyst > 0,
           "La identidad del club debe completar el staff ampliado.");
}

void testManagerInboxTracksNewsAndScouting() {
    Career career;
    career.currentSeason = 1;
    career.currentWeek = 3;
    career.allTeams.push_back(makeTeam("Inbox United", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 800000));
    career.allTeams.push_back(makeTeam("Region Sur FC", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 620000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Inbox United");
    expect(career.myTeam != nullptr, "La prueba de inbox necesita club usuario.");

    career.myTeam->scoutingRegions = {"Metropolitana", "Sur"};
    career.findTeamByName("Region Sur FC")->youthRegion = "Sur";
    career.addNews("[Mundo] Prueba de noticia prioritaria.");
    const size_t before = career.managerInbox.size();
    const ScoutingSessionResult session = runScoutingSessionService(career, "Sur", "DEF");
    expect(session.service.ok, "La sesion de scouting de prueba debe completarse.");
    expect(career.managerInbox.size() > before,
           "El inbox del manager debe crecer con noticias y scouting relevante.");
}

void testConfiguredWorldDataLoads() {
    const int managerChangeChance = world_state_service::worldRuleValue("manager_change_chance", 0);
    const vector<string> regions = world_state_service::listConfiguredScoutingRegions();
    expect(managerChangeChance > 0, "Las reglas de mundo configuradas deben cargarse desde CSV o fallback.");
    expect(find(regions.begin(), regions.end(), "Internacional") != regions.end(),
           "La lista configurable de regiones debe exponer la cobertura internacional.");
}

void testLeagueRegistryLoadsConfiguredData() {
    const vector<DivisionInfo>& divisions = listRegisteredDivisions();
    expect(!divisions.empty(), "El registro de ligas debe exponer divisiones configuradas.");
    expect(divisions.front().folder.find("LigaChilena") != string::npos,
           "El registro de ligas debe entregar carpetas de datos reales.");
    expect(registeredCompetitionRulesPath().find("competition_rules.csv") != string::npos,
           "El registro de ligas debe exponer la ruta de reglas de competicion.");
}

void testAnalyticsAndInboxServicesProduceUsefulBlocks() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 5;
    career.allTeams.push_back(makeTeam("Analitica FC", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 800000));
    career.allTeams.push_back(makeTeam("Rival Analitico", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 620000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Analitica FC");
    expect(career.myTeam != nullptr, "La prueba de analitica necesita club usuario.");
    career.lastMatchCenter.opponentName = "Rival Analitico";
    career.lastMatchCenter.myGoals = 2;
    career.lastMatchCenter.oppGoals = 1;
    career.lastMatchCenter.myExpectedGoalsTenths = 16;
    career.lastMatchCenter.oppExpectedGoalsTenths = 8;
    career.lastMatchCenter.tacticalSummary = "Se gano la espalda del rival.";
    career.lastMatchCenter.fatigueSummary = "El lateral derecho termino cargado.";
    career.addInboxItem("Hay revision de contratos en tres puestos.", "Directiva");
    career.scoutInbox.push_back("Scout: aparece una opcion joven en el sur.");

    const auto analytics = analytics_service::buildTeamAnalyticsLines(career, *career.myTeam);
    const auto inbox = inbox_service::buildInboxSummaryLines(career, 4);
    expect(!analytics.empty() && analytics.front().find("Indice ofensivo") != string::npos,
           "El data hub debe producir lineas utiles para reportes y GUI.");
    expect(!inbox.empty() && inbox.front().find("|") != string::npos,
           "El inbox combinado debe resumir canal y contenido.");
}

void testGameSettingsCycleAndDifficultyImpact() {
    GameSettings settings;
    settings.volume = 70;
    expect(game_settings::nextVolume(settings.volume) == 75,
           "El volumen debe ciclar al siguiente escalon previsto.");
    game_settings::cycleDifficulty(settings);
    expect(settings.difficulty == GameDifficulty::Challenging,
           "La dificultad debe avanzar al siguiente nivel.");
    game_settings::cycleSimulationSpeed(settings);
    expect(settings.simulationSpeed == SimulationSpeed::Rapid,
           "La velocidad de simulacion debe avanzar al siguiente ritmo previsto.");
    game_settings::cycleSimulationMode(settings);
    expect(settings.simulationMode == SimulationMode::Fast,
           "El modo de simulacion debe alternar entre detallado y rapido.");
    expect(game_settings::settingsSummary(settings).find("Velocidad") != string::npos,
           "El resumen de configuracion debe incluir la velocidad de simulacion.");

    Career career;
    career.boardConfidence = 52;
    career.managerReputation = 50;
    career.allTeams.push_back(makeTeam("Proyecto Ligero", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 600000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Proyecto Ligero");
    expect(career.myTeam != nullptr, "La prueba de settings necesita club usuario.");

    const long long baseBudget = career.myTeam->budget;
    settings.difficulty = GameDifficulty::Accessible;
    game_settings::applyNewCareerDifficulty(career, settings);

    expect(career.myTeam->budget > baseBudget,
           "La dificultad accesible debe dar algo mas de aire economico al arranque.");
    expect(career.boardConfidence > 52,
           "La dificultad accesible debe mejorar el respaldo inicial de la directiva.");
}

void testGameSettingsPersistenceAndFrontendScope() {
    const string settingsPath = "saves/test_game_settings.cfg";
    std::remove(settingsPath.c_str());

    GameSettings original;
    original.volume = 25;
    original.difficulty = GameDifficulty::Challenging;
    original.simulationSpeed = SimulationSpeed::Relaxed;
    original.simulationMode = SimulationMode::Fast;
    original.language = UiLanguage::English;
    original.textSpeed = TextSpeed::Rapid;
    original.visualProfile = VisualProfile::HighContrast;
    original.menuMusicMode = MenuMusicMode::FrontendPages;
    original.menuAudioFade = false;
    original.menuThemeId = "el-crack";

    expect(game_settings::saveToDisk(original, settingsPath),
           "La configuracion del frontend debe persistirse en disco.");

    GameSettings loaded;
    loaded.volume = 100;
    loaded.difficulty = GameDifficulty::Accessible;
    loaded.simulationSpeed = SimulationSpeed::Rapid;
    loaded.simulationMode = SimulationMode::Detailed;
    loaded.language = UiLanguage::Spanish;
    loaded.textSpeed = TextSpeed::Relaxed;
    loaded.visualProfile = VisualProfile::Broadcast;
    loaded.menuMusicMode = MenuMusicMode::Off;
    loaded.menuAudioFade = true;
    loaded.menuThemeId = "otro";

    expect(game_settings::loadFromDisk(loaded, settingsPath),
           "La configuracion persistida debe volver a cargarse.");
    expect(loaded.volume == 25, "El volumen debe sobrevivir entre sesiones.");
    expect(loaded.difficulty == GameDifficulty::Challenging, "La dificultad persistida debe restaurarse.");
    expect(loaded.simulationSpeed == SimulationSpeed::Relaxed, "La velocidad persistida debe restaurarse.");
    expect(loaded.simulationMode == SimulationMode::Fast, "El modo de simulacion persistido debe restaurarse.");
    expect(loaded.language == UiLanguage::English, "El idioma persistido debe restaurarse.");
    expect(loaded.textSpeed == TextSpeed::Rapid, "La velocidad de texto persistida debe restaurarse.");
    expect(loaded.visualProfile == VisualProfile::HighContrast, "El perfil visual persistido debe restaurarse.");
    expect(loaded.menuMusicMode == MenuMusicMode::FrontendPages, "El alcance de musica persistido debe restaurarse.");
    expect(!loaded.menuAudioFade, "La preferencia de fade de audio debe sobrevivir entre sesiones.");
    expect(game_settings::shouldPlayMenuMusic(loaded, false, true),
           "El modo de musica extendido debe habilitar audio en otras pantallas del frontend.");
    expect(!game_settings::shouldPlayMenuMusic(loaded, false, false),
           "El audio del menu no debe salir del frontend.");
    expect(game_settings::uiPulseDelayMs(loaded) > game_settings::pageTransitionDelayMs(loaded),
           "El pulso general debe ser mas lento que la transicion de pagina.");

    std::remove(settingsPath.c_str());
}

void testLegacyDivisionIdentifiersCanonicalizeOnLoad() {
    Career original;
    original.currentSeason = 3;
    original.currentWeek = 11;
    original.managerName = "Moises";
    original.boardConfidence = 61;
    original.allTeams.push_back(makeTeam("Audax Italiano", "primera division", 71, 3, 3, "Balanced", "Equilibrado", 700000));
    original.allTeams.push_back(makeTeam("Rival Clasico", "primera division", 68, 2, 2, "Defensive", "Bloque bajo", 620000));
    original.setActiveDivision("primera division");
    original.myTeam = original.findTeamByName("Audax Italiano");
    expect(original.myTeam != nullptr, "La prueba de migracion de divisiones necesita club controlado.");

    ostringstream encoded;
    expect(save_serialization::serializeCareer(encoded, original),
           "La carrera base debe serializarse antes de simular un save legacy.");

    string legacySave = replaceAllCopy(encoded.str(), "primera division", "Primera Division");
    istringstream input(legacySave);
    Career loaded;
    loaded.saveFile = "saves/legacy_division_test.txt";
    expect(save_serialization::deserializeCareer(input, loaded),
           "El cargador debe aceptar identificadores legacy de division.");
    expect(loaded.activeDivision == "primera division",
           "La division activa debe migrarse al ID canonico.");
    expect(!loaded.divisions.empty() && loaded.divisions.front().id == "primera division",
           "La lista de divisiones cargadas debe reconstruirse con IDs canonicos.");
    expect(std::all_of(loaded.allTeams.begin(), loaded.allTeams.end(), [](const Team& team) {
               return team.division == "primera division";
           }),
           "Los equipos del save legacy deben quedar normalizados al ID canonico.");
    expect(loaded.myTeam != nullptr && loaded.myTeam->division == "primera division",
           "El club usuario debe mantener la division canonica tras la migracion.");
}

void testManagerHubDigestCombinesStaffAndAgenda() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 7;
    career.boardConfidence = 33;
    career.boardMonthlyObjective = "Entrar a puestos altos";
    career.boardMonthlyTarget = 3;
    career.boardMonthlyProgress = 1;
    career.allTeams.push_back(makeTeam("Hub FC", "primera division", 69, 4, 4, "Pressing", "Contra-presion", 180000));
    career.allTeams.push_back(makeTeam("Radar Sur", "primera division", 67, 2, 2, "Defensive", "Bloque bajo", 640000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Hub FC");
    expect(career.myTeam != nullptr, "La prueba del hub necesita club usuario.");

    career.myTeam->morale = 44;
    career.myTeam->debt = 420000;
    career.myTeam->players[0].contractWeeks = 8;
    career.myTeam->players[0].fitness = 53;
    career.myTeam->players[0].fatigueLoad = 76;
    career.myTeam->players[1].happiness = 39;
    career.scoutingShortlist.push_back("Radar Sur|" + career.findTeamByName("Radar Sur")->players[0].name);
    career.addInboxItem("La directiva pide reaccion inmediata.", "Directiva");

    const auto priority = inbox_service::buildPriorityInboxLines(career, 6);
    const string digest = inbox_service::buildManagerHubDigest(career, 6);

    expect(!priority.empty(), "El hub del manager debe producir lineas priorizadas.");
    expect(joinLines(priority).find("Staff |") != string::npos,
           "El hub debe mezclar alertas del staff.");
    expect(joinLines(priority).find("Agenda |") != string::npos,
           "El hub debe mezclar acciones del manager.");
    expect(digest.find("Centro del manager") != string::npos && digest.find("Staff |") != string::npos,
           "El digest del hub debe quedar legible para GUI e inbox.");
}

void testManagerAdviceHighlightsUrgentActions() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 6;
    career.boardConfidence = 34;
    career.boardMonthlyObjective = "Sumar al menos 6 puntos en 4 semanas";
    career.boardMonthlyTarget = 6;
    career.boardMonthlyProgress = 2;
    career.allTeams.push_back(makeTeam("Plan FC", "primera division", 68, 4, 4, "Pressing", "Contra-presion", 120000));
    career.allTeams.push_back(makeTeam("Rival Sur", "primera division", 67, 2, 2, "Defensive", "Bloque bajo", 620000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Plan FC");
    expect(career.myTeam != nullptr, "La prueba de recomendaciones necesita club usuario.");

    career.myTeam->debt = 500000;
    career.myTeam->morale = 42;
    career.myTeam->players[0].contractWeeks = 8;
    career.myTeam->players[1].contractWeeks = 10;
    career.myTeam->players[0].fitness = 54;
    career.myTeam->players[0].fatigueLoad = 74;
    career.myTeam->players[1].happiness = 36;
    career.activePromises.push_back({career.myTeam->players[0].name, "Minutos", "Titular", 4, 8, 1, false, false});

    const auto advice = manager_advice::buildManagerActionLines(career, 6);
    const string dump = joinLines(advice);
    expect(dump.find("Renovar") != string::npos,
           "Las recomendaciones deben detectar contratos cortos.");
    expect(dump.find("vestuario") != string::npos || dump.find("Vestuario") != string::npos,
           "Las recomendaciones deben detectar gestion emocional urgente.");
    expect(dump.find("Directiva") != string::npos,
           "Las recomendaciones deben detectar presion de directiva.");
}

void testTransferBriefingBuildsActionableMarketView() {
    Career career;
    career.currentSeason = 2;
    career.currentWeek = 4;
    career.managerReputation = 72;
    career.scoutingShortlist.push_back("Mercado Norte|Objetivo Creativo");

    career.allTeams.push_back(makeTeam("Mercado Local", "primera division", 66, 3, 3, "Balanced", "Equilibrado", 950000));
    career.allTeams.push_back(makeTeam("Mercado Norte", "primera division", 72, 3, 3, "Balanced", "Equilibrado", 820000));
    career.allTeams.push_back(makeTeam("Mercado Sur", "primera division", 70, 2, 2, "Defensive", "Bloque bajo", 760000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Mercado Local");
    expect(career.myTeam != nullptr, "La prueba de briefing de mercado necesita club usuario.");

    Team* seller = career.findTeamByName("Mercado Norte");
    expect(seller != nullptr, "La prueba de briefing de mercado necesita club vendedor.");
    seller->players.clear();

    Player target = makePlayer("Objetivo Creativo", "MED", 76, 84, 22, 74, 76);
    target.value = 420000;
    target.releaseClause = 760000;
    target.wage = 18000;
    target.contractWeeks = 16;
    target.currentForm = 72;
    seller->addPlayer(target);

    Player loanTarget = makePlayer("Proyecto Cedido", "MED", 71, 83, 20, 75, 77);
    loanTarget.value = 360000;
    loanTarget.releaseClause = 700000;
    loanTarget.wage = 11000;
    loanTarget.contractWeeks = 92;
    seller->addPlayer(loanTarget);

    const auto options = transfer_briefing::buildTransferOptions(career, "MED", true, 5);
    expect(!options.empty(), "El briefing de mercado debe devolver objetivos.");
    expect(!options.front().competitionLabel.empty() &&
               !options.front().actionLabel.empty() &&
               !options.front().packageLabel.empty(),
           "Cada objetivo debe bajar el mercado a competencia, accion y paquete.");

    const auto pulse = transfer_briefing::buildMarketPulseLines(career, 4);
    expect(!pulse.empty(), "El mercado debe devolver una lectura resumida para la semana.");

    const auto opportunities = transfer_briefing::buildTransferOpportunityLines(career, "MED", 2);
    expect(!opportunities.empty() && opportunities.front().find("Entrada") != string::npos,
           "Las oportunidades resumidas deben incluir el paquete economico recomendado.");
}

void testLateDeficitRaisesUrgencyInMatchPhase() {
    Team home = makeTeam("Urgencia Local", "primera division", 71, 3, 3, "Balanced", "Equilibrado", 780000);
    Team away = makeTeam("Urgencia Visitante", "primera division", 71, 3, 3, "Balanced", "Equilibrado", 780000);

    setRandomSeed(20260325);
    const MatchSetup setup = match_context::buildMatchSetup(home, away, false, false);
    setRandomSeed(4242);
    const MatchPhaseEvaluation even =
        match_phase::evaluatePhase(setup, home, away, setup.home, setup.away, 5, 76, 90, 0, 0, 11, 11);
    setRandomSeed(4242);
    const MatchPhaseEvaluation chasing =
        match_phase::evaluatePhase(setup, home, away, setup.home, setup.away, 5, 76, 90, 0, 1, 11, 11);
    resetRandomSeed();

    expect(chasing.report.homeUrgency > even.report.homeUrgency,
           "Ir perdiendo al final debe aumentar la urgencia del local.");
    expect(chasing.homeAttack >= even.homeAttack || chasing.homeAttacks >= even.homeAttacks,
           "El contexto de remontada debe empujar el volumen ofensivo.");
    expect(chasing.report.homeDefensiveRisk >= even.report.homeDefensiveRisk,
           "Empujar el resultado debe venir con mas riesgo defensivo.");
}

void testMatchInstructionsShapeChanceProfiles() {
    Team wide = makeTeam("Bandas FC", "primera division", 71, 3, 3, "Balanced", "Por bandas", 780000);
    wide.width = 5;
    Team patient = wide;
    patient.name = "Pausa FC";
    patient.matchInstruction = "Pausar juego";
    patient.width = 3;
    Team opponent = makeTeam("Rival Medio", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 760000);

    setRandomSeed(20260326);
    const MatchSetup wideSetup = match_context::buildMatchSetup(wide, opponent, false, false);
    setRandomSeed(20260326);
    const MatchSetup patientSetup = match_context::buildMatchSetup(patient, opponent, false, false);

    setRandomSeed(7777);
    const MatchPhaseEvaluation wideEval =
        match_phase::evaluatePhase(wideSetup, wide, opponent, wideSetup.home, wideSetup.away, 2, 31, 45, 0, 0, 11, 11);
    setRandomSeed(7777);
    const MatchPhaseEvaluation patientEval =
        match_phase::evaluatePhase(patientSetup, patient, opponent, patientSetup.home, patientSetup.away, 2, 31, 45, 0, 0, 11, 11);
    resetRandomSeed();

    expect(wideEval.report.homeChanceProbability > patientEval.report.homeChanceProbability,
           "Atacar por bandas debe generar mas probabilidad de ocasiones que pausar el juego.");
    expect(wideEval.homeAttacks >= patientEval.homeAttacks,
           "El plan por bandas debe sostener al menos el mismo volumen ofensivo.");
}

void testMatchInjuryTriggersRealReplacement() {
    Team team = makeTeam("Lesionados FC", "primera division", 70, 4, 4, "Pressing", "Contra-presion", 820000);
    Team opponent = makeTeam("Rival Control", "primera division", 69, 3, 3, "Balanced", "Equilibrado", 810000);
    (void)opponent;

    vector<int> activeXI = team.getStartingXIIndices();
    expect(!activeXI.empty(), "La prueba de lesion necesita once inicial.");
    const int riskyIndex = activeXI.front();
    team.players[static_cast<size_t>(riskyIndex)].fitness = 34;
    team.players[static_cast<size_t>(riskyIndex)].fatigueLoad = 88;
    team.players[static_cast<size_t>(riskyIndex)].injuryHistory = 5;
    ensurePlayerProfile(team.players[static_cast<size_t>(riskyIndex)], true);

    vector<int> participants = activeXI;
    MatchTimeline timeline;
    vector<string> injuredPlayers;

    setRandomSeed(20260325);
    match_event_generator::maybeInjure(team, activeXI, participants, 1.0, 30, 45, timeline, injuredPlayers);
    resetRandomSeed();

    expect(!injuredPlayers.empty(), "El motor debe registrar la lesion forzada.");
    expect(find(activeXI.begin(), activeXI.end(), riskyIndex) == activeXI.end(),
           "El jugador lesionado no debe seguir activo en el XI.");
    expect(participants.size() >= 12,
           "Una lesion con banca disponible debe registrar un reemplazo.");
    expect(any_of(timeline.events.begin(), timeline.events.end(), [](const MatchEvent& event) {
               return event.type == MatchEventType::Substitution;
           }),
           "La cronologia debe reflejar el cambio obligado por lesion.");
}

void testTransferEvaluationPenalizesMedicalRisk() {
    Career career;
    Team buyer = makeTeam("Comprador Saludable", "primera division", 72, 3, 3, "Balanced", "Equilibrado", 900000);
    Team seller = makeTeam("Vendedor Test", "primera division", 68, 3, 3, "Balanced", "Equilibrado", 700000);
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, buyer);

    Player healthy = makePlayer("Creador Sano", "MED", 74, 84, 23, 76, 78);
    healthy.contractWeeks = 40;
    healthy.value = 420000;
    healthy.releaseClause = 820000;
    healthy.wage = 17000;

    Player risky = healthy;
    risky.name = "Creador Fragil";
    risky.fitness = 44;
    risky.fatigueLoad = 82;
    risky.injuryHistory = 5;
    ensurePlayerProfile(risky, true);

    const TransferTarget healthyTarget = ai_transfer_manager::evaluateTarget(career, buyer, seller, healthy, strategy);
    const TransferTarget riskyTarget = ai_transfer_manager::evaluateTarget(career, buyer, seller, risky, strategy);

    expect(riskyTarget.medicalRisk > healthyTarget.medicalRisk,
           "El target fragil debe reflejar mas riesgo medico.");
    expect(riskyTarget.totalScore < healthyTarget.totalScore,
           "El mercado debe penalizar perfiles con mayor riesgo fisico.");
}

void testMonthlyDevelopmentCycleImprovesStableProspects() {
    Team team = makeTeam("Cantera Pro", "primera division", 67, 3, 3, "Balanced", "Equilibrado", 860000);
    team.youthCoach = 82;
    team.trainingFacilityLevel = 4;
    team.youthFacilityLevel = 4;
    team.performanceAnalyst = 78;
    team.assistantCoach = 76;

    vector<int> originalSkills;
    originalSkills.reserve(team.players.size());
    for (size_t i = 0; i < team.players.size(); ++i) {
        Player& player = team.players[i];
        if (i < 4) {
            player.age = 18 + static_cast<int>(i % 2);
            player.skill = 60 + static_cast<int>(i);
            player.potential = player.skill + 16;
            player.professionalism = 82;
            player.happiness = 72;
            player.matchesPlayed = 6;
            player.startsThisSeason = 4;
            player.currentForm = 68;
            player.fatigueLoad = 18;
            player.fitness = 78;
            player.developmentPlan = (i % 2 == 0) ? "Creatividad" : "Finalizacion";
            applyPositionStats(player);
            ensurePlayerProfile(player, true);
        }
        originalSkills.push_back(player.skill);
    }

    setRandomSeed(20260325);
    const development::MonthlyDevelopmentSummary summary = development::runMonthlyDevelopmentCycle(team, 20);
    resetRandomSeed();

    const bool anyImproved = any_of(team.players.begin(), team.players.end(), [&](const Player& player) {
        const ptrdiff_t index = &player - team.players.data();
        return index >= 0 && index < static_cast<ptrdiff_t>(originalSkills.size()) &&
               player.skill > originalSkills[static_cast<size_t>(index)];
    });

    expect(summary.improvedPlayers > 0 && anyImproved,
           "La progresion mensual debe empujar al menos a un prospecto estable.");
}

void testMatchCenterAddsRecommendedAdjustments() {
    Career career;
    career.lastMatchAnalysis = "Partido denso y largo.";
    career.lastMatchPlayerOfTheMatch = "Figura Test";
    career.lastMatchCenter.competitionLabel = "Liga";
    career.lastMatchCenter.opponentName = "Rival Tactico";
    career.lastMatchCenter.venueLabel = "Visita";
    career.lastMatchCenter.myGoals = 0;
    career.lastMatchCenter.oppGoals = 2;
    career.lastMatchCenter.myShots = 6;
    career.lastMatchCenter.oppShots = 15;
    career.lastMatchCenter.myShotsOnTarget = 1;
    career.lastMatchCenter.oppShotsOnTarget = 7;
    career.lastMatchCenter.myPossession = 39;
    career.lastMatchCenter.oppPossession = 61;
    career.lastMatchCenter.myExpectedGoalsTenths = 7;
    career.lastMatchCenter.oppExpectedGoalsTenths = 18;
    career.lastMatchCenter.weather = "Lluvia";
    career.lastMatchCenter.tacticalSummary = "El rival encontro la espalda.";
    career.lastMatchCenter.fatigueSummary = "El equipo llego roto al cierre.";
    career.lastMatchCenter.postMatchImpact = "Moral -4";
    career.lastMatchCenter.phaseSummaries = {"1-15: el rival rompe por fuera"};

    const MatchCenterView view = match_center_service::buildLastMatchCenter(career, 3, 3);
    expect(view.available, "El match center debe quedar disponible con snapshot cargado.");
    expect(!view.recommendationLines.empty(),
           "El match center debe devolver ajustes sugeridos cuando el partido deja problemas claros.");
    const string dump = joinLines(view.recommendationLines);
    expect(dump.find("Proteger mejor el area") != string::npos || dump.find("volumen ofensivo") != string::npos,
           "Los ajustes sugeridos deben explicar que corregir tras el partido.");
}

void testIgnoredArchivedFoldersConfigSuppressesKnownWarnings() {
    const DataValidationReport report = buildRosterDataValidationReport();
    const string issueDump = joinLines(report.lines);
    expect(issueDump.find("AC Barnechea") == string::npos,
           "Las carpetas historicas ignoradas no deben seguir apareciendo como advertencias activas.");
}

void testTeamsTxtFolderAliasesDoNotTriggerOrphanWarnings() {
    const DataValidationReport report = buildRosterDataValidationReport();
    const string issueDump = joinLines(report.lines);
    expect(issueDump.find("Integridad liga | tercera division a | Futuro |") == string::npos,
           "Las entradas Display|Folder de teams.txt no deben marcar carpetas activas como huerfanas.");
    expect(issueDump.find("Integridad liga | tercera division b | Audax Paipote |") == string::npos,
           "Las entradas con alias de carpeta en teams.txt deben reconocerse en la auditoria.");
    expect(issueDump.find("Integridad liga | tercera division b | Rep?blica Ind. de Hualqui |") == string::npos,
           "Los nombres de carpeta configurados explicitamente en teams.txt deben evitar falsos positivos de integridad.");
}

void testSaveCareerCreatesBackup() {
    const string savePath = "saves/test_backup_save.txt";
    Career career;
    career.currentSeason = 3;
    career.currentWeek = 4;
    career.allTeams.push_back(makeTeam("Backup FC", "primera division", 70, 3, 3, "Balanced", "Equilibrado", 700000));
    career.allTeams.push_back(makeTeam("Backup Rival", "primera division", 66, 2, 2, "Balanced", "Equilibrado", 620000));
    career.setActiveDivision("primera division");
    career.myTeam = career.findTeamByName("Backup FC");
    expect(career.myTeam != nullptr, "La prueba de backup necesita club usuario.");
    career.saveFile = savePath;

    expect(career.saveCareer(), "El primer guardado debe completarse.");
    career.currentWeek = 5;
    expect(career.saveCareer(), "El segundo guardado debe completarse.");
    expect(pathExists(savePath + ".bak"),
           "El segundo guardado debe dejar un backup del save previo.");

    std::remove(savePath.c_str());
    std::remove((savePath + ".bak").c_str());
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
        {"game_settings_cycle", testGameSettingsCycleAndDifficultyImpact},
        {"game_settings_persistence", testGameSettingsPersistenceAndFrontendScope},
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
        {"dressing_room_tension", testDressingRoomSnapshotTracksSocialTension},
        {"scouting_confidence", testScoutingConfidenceReflectsStaffQuality},
        {"world_state_seed", testWorldStateSeedsPromisesAndRecords},
        {"world_state_promises", testWeeklyWorldStateResolvesMinutesPromise},
        {"season_promise_carryover", testSeasonTransitionResolvesCarryoverPromises},
        {"training_microcycle", testWeeklyTrainingScheduleAdaptsToCongestion},
        {"team_meeting", testTeamMeetingImprovesMorale},
        {"player_instruction_service", testPlayerInstructionServiceCyclesInstruction},
        {"inbox_medical_decision", testInboxDecisionPrioritizesRecovery},
        {"staff_review", testStaffReviewImprovesWeakestArea},
        {"scouting_assignments", testScoutingAssignmentShowsInReport},
        {"role_duty_snapshot", testRoleDutyShapesMatchSnapshot},
        {"club_world_metadata", testTeamIdentitySeedsWorldMetadata},
        {"manager_inbox", testManagerInboxTracksNewsAndScouting},
        {"configured_world_data", testConfiguredWorldDataLoads},
        {"league_registry_data", testLeagueRegistryLoadsConfiguredData},
        {"analytics_inbox_blocks", testAnalyticsAndInboxServicesProduceUsefulBlocks},
        {"manager_hub_digest", testManagerHubDigestCombinesStaffAndAgenda},
        {"manager_advice", testManagerAdviceHighlightsUrgentActions},
        {"transfer_briefing", testTransferBriefingBuildsActionableMarketView},
        {"late_match_urgency", testLateDeficitRaisesUrgencyInMatchPhase},
        {"instruction_chance_profiles", testMatchInstructionsShapeChanceProfiles},
        {"injury_replacement", testMatchInjuryTriggersRealReplacement},
        {"transfer_medical_risk", testTransferEvaluationPenalizesMedicalRisk},
        {"monthly_development", testMonthlyDevelopmentCycleImprovesStableProspects},
        {"match_center_adjustments", testMatchCenterAddsRecommendedAdjustments},
        {"ignored_archived_folders", testIgnoredArchivedFoldersConfigSuppressesKnownWarnings},
        {"teams_txt_folder_aliases", testTeamsTxtFolderAliasesDoNotTriggerOrphanWarnings},
        {"negotiation_agent_costs", testNegotiationTracksAgentCosts},
        {"save_backup", testSaveCareerCreatesBackup},
        {"simulate_match_state", testSimulateMatchAppliesPostProcessState},
        {"save_load_roundtrip", testSaveLoadRoundTripPreservesCareerState},
        {"legacy_division_ids", testLegacyDivisionIdentifiersCanonicalizeOnLoad},
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
