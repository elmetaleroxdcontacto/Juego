#include "simulation/match_engine.h"

#include "ai/ai_match_manager.h"
#include "simulation/fatigue_engine.h"
#include "simulation/match_context.h"
#include "simulation/match_resolution.h"
#include "simulation/tactics_engine.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;

namespace {

struct TeamRuntimeState {
    Team team;
    vector<int> xi;
    vector<int> participants;
    vector<string> cautionedPlayers;
    vector<string> sentOffPlayers;
    vector<string> injuredPlayers;
    vector<GoalContribution> goals;
};

const vector<pair<int, int>> kPhases = {{1, 15}, {16, 30}, {31, 45}, {46, 60}, {61, 75}, {76, 90}};

void applyImpact(MatchStats& stats, const MatchEventImpact& impact) {
    stats.homeGoals += impact.homeGoalsDelta;
    stats.awayGoals += impact.awayGoalsDelta;
    stats.homeShots += impact.homeShotsDelta;
    stats.awayShots += impact.awayShotsDelta;
    stats.homeShotsOnTarget += impact.homeShotsOnTargetDelta;
    stats.awayShotsOnTarget += impact.awayShotsOnTargetDelta;
    stats.homeCorners += impact.homeCornersDelta;
    stats.awayCorners += impact.awayCornersDelta;
    stats.homeFouls += impact.homeFoulsDelta;
    stats.awayFouls += impact.awayFoulsDelta;
    stats.homeYellowCards += impact.homeYellowCardsDelta;
    stats.awayYellowCards += impact.awayYellowCardsDelta;
    stats.homeRedCards += impact.homeRedCardsDelta;
    stats.awayRedCards += impact.awayRedCardsDelta;
    stats.homeExpectedGoals += impact.homeExpectedGoalsDelta;
    stats.awayExpectedGoals += impact.awayExpectedGoalsDelta;
}

void pushEvent(MatchTimeline& timeline, MatchStats& stats, const MatchEvent& event) {
    timeline.events.push_back(event);
    applyImpact(stats, event.impact);
}

int pickPhysicalRiskPlayer(const TeamRuntimeState& state) {
    int bestIdx = -1;
    int bestScore = -1;
    for (int idx : state.xi) {
        if (idx < 0 || idx >= static_cast<int>(state.team.players.size())) continue;
        const Player& player = state.team.players[static_cast<size_t>(idx)];
        int score = (100 - player.fitness) * 2 + max(0, player.age - 29) * 3 + player.injuryHistory * 4;
        if (playerHasTrait(player, "Fragil")) score += 10;
        if (score > bestScore) {
            bestScore = score;
            bestIdx = idx;
        }
    }
    return bestIdx;
}

string formatDouble2(double value) {
    ostringstream out;
    out.setf(ios::fixed);
    out.precision(2);
    out << value;
    return out.str();
}

vector<string> buildLegacyTimeline(const MatchTimeline& timeline) {
    vector<MatchEvent> ordered = timeline.events;
    sort(ordered.begin(), ordered.end(), [](const MatchEvent& left, const MatchEvent& right) {
        if (left.minute != right.minute) return left.minute < right.minute;
        return static_cast<int>(left.type) < static_cast<int>(right.type);
    });

    vector<string> lines;
    for (const MatchEvent& event : ordered) {
        ostringstream out;
        out << event.minute << "' ";
        if (!event.teamName.empty()) out << event.teamName << ": ";
        if (!event.playerName.empty() && event.description.find(event.playerName) == string::npos) {
            out << event.playerName << " ";
        }
        out << event.description;
        lines.push_back(out.str());
    }
    return lines;
}

string buildProbableReason(const MatchSetup& setup, const MatchStats& stats, const Team& home, const Team& away) {
    if (stats.homeExpectedGoals > stats.awayExpectedGoals + 0.35) {
        return home.name + " genero ocasiones mas limpias y aprovecho mejor sus fases dominantes.";
    }
    if (stats.awayExpectedGoals > stats.homeExpectedGoals + 0.35) {
        return away.name + " encontro mejores ventanas de remate y castigo la estructura rival.";
    }
    if (setup.context.midfieldControlHome > setup.context.midfieldControlAway + 4.0) {
        return home.name + " controlo mejor el mediocampo y redujo la respuesta rival.";
    }
    if (setup.context.midfieldControlAway > setup.context.midfieldControlHome + 4.0) {
        return away.name + " gano el centro del campo y sostuvo su plan con mas continuidad.";
    }
    if (setup.context.tacticalAdvantageHome > setup.context.tacticalAdvantageAway + 0.08) {
        return home.name + " encontro un emparejamiento tactico mas favorable.";
    }
    if (setup.context.tacticalAdvantageAway > setup.context.tacticalAdvantageHome + 0.08) {
        return away.name + " saco ventaja del planteamiento y del contexto del partido.";
    }
    return "El resultado se definio por detalles dentro de un tramite parejo.";
}

}  // namespace

namespace match_engine {

MatchSimulationData simulate(const Team& home, const Team& away, bool keyMatch, bool neutralVenue) {
    MatchSimulationData data;
    const MatchSetup setup = match_context::buildMatchSetup(home, away, keyMatch, neutralVenue);

    TeamRuntimeState homeState{home, setup.home.xi, setup.home.xi};
    TeamRuntimeState awayState{away, setup.away.xi, setup.away.xi};
    MatchStats stats;
    MatchTimeline timeline;
    int homePossAccumulator = 0;

    for (size_t phaseIndex = 0; phaseIndex < kPhases.size(); ++phaseIndex) {
        const int minuteStart = kPhases[phaseIndex].first;
        const int minuteEnd = kPhases[phaseIndex].second;

        MatchPhaseReport phase;
        phase.minuteStart = minuteStart;
        phase.minuteEnd = minuteEnd;

        phase.homeTacticalChange = ai_match_manager::applyInMatchManagement(homeState.team,
                                                                            awayState.team,
                                                                            homeState.xi,
                                                                            homeState.participants,
                                                                            homeState.cautionedPlayers,
                                                                            minuteEnd,
                                                                            stats.homeGoals,
                                                                            stats.awayGoals,
                                                                            timeline);
        phase.awayTacticalChange = ai_match_manager::applyInMatchManagement(awayState.team,
                                                                            homeState.team,
                                                                            awayState.xi,
                                                                            awayState.participants,
                                                                            awayState.cautionedPlayers,
                                                                            minuteEnd,
                                                                            stats.awayGoals,
                                                                            stats.homeGoals,
                                                                            timeline);

        const TeamMatchSnapshot homeSnapshot = match_context::rebuildSnapshot(homeState.team, awayState.team, homeState.xi, keyMatch);
        const TeamMatchSnapshot awaySnapshot = match_context::rebuildSnapshot(awayState.team, homeState.team, awayState.xi, keyMatch);
        const int homePossessionShare = clampInt(
            static_cast<int>(round(50.0 + (homeSnapshot.midfieldControl - awaySnapshot.midfieldControl) / 3.4 +
                                   (tactics_engine::possessionWeight(homeSnapshot.tacticalProfile) -
                                    tactics_engine::possessionWeight(awaySnapshot.tacticalProfile)) *
                                       10.0)),
            32, 68);
        const int awayPossessionShare = 100 - homePossessionShare;
        const double intensity =
            clampValue(0.92 + phaseIndex * 0.04 +
                           (homeSnapshot.tacticalProfile.tempo + awaySnapshot.tacticalProfile.tempo) * 0.22 +
                           (homeSnapshot.tacticalProfile.pressing + awaySnapshot.tacticalProfile.pressing) * 0.15,
                       0.90, 1.65);

        const double homeAttack = max(12.0, (homeSnapshot.attackPower * homeSnapshot.moraleFactor * homeSnapshot.fatigueFactor +
                                             homeSnapshot.midfieldControl * 0.55 + homeSnapshot.tacticalAdvantage * 20.0) *
                                                setup.context.homeAdvantage * setup.context.weatherModifier);
        const double awayAttack = max(12.0, (awaySnapshot.attackPower * awaySnapshot.moraleFactor * awaySnapshot.fatigueFactor +
                                             awaySnapshot.midfieldControl * 0.55 + awaySnapshot.tacticalAdvantage * 20.0) *
                                                setup.context.weatherModifier);
        const double homeDefense = max(10.0, homeSnapshot.defensePower * (0.94 + homeSnapshot.fatigueFactor * 0.08) +
                                                 homeSnapshot.midfieldControl * 0.28 +
                                                 tactics_engine::defensiveSecurityWeight(homeSnapshot.tacticalProfile) * 12.0);
        const double awayDefense = max(10.0, awaySnapshot.defensePower * (0.94 + awaySnapshot.fatigueFactor * 0.08) +
                                                 awaySnapshot.midfieldControl * 0.28 +
                                                 tactics_engine::defensiveSecurityWeight(awaySnapshot.tacticalProfile) * 12.0);

        const double homeOpportunity = match_resolution::opportunityProbability(homeAttack,
                                                                                awayDefense,
                                                                                homePossessionShare / 100.0,
                                                                                0.95 + homeSnapshot.tacticalProfile.mentality * 0.10,
                                                                                intensity);
        const double awayOpportunity = match_resolution::opportunityProbability(awayAttack,
                                                                                homeDefense,
                                                                                awayPossessionShare / 100.0,
                                                                                0.95 + awaySnapshot.tacticalProfile.mentality * 0.10,
                                                                                intensity);

        phase.dominantTeam = homePossessionShare >= awayPossessionShare ? home.name : away.name;
        phase.homePossessionShare = homePossessionShare;
        phase.awayPossessionShare = awayPossessionShare;
        phase.intensity = intensity;
        phase.homeChanceProbability = homeOpportunity;
        phase.awayChanceProbability = awayOpportunity;
        phase.homeFatigueGain = fatigue_engine::phaseFatigueGain(homeState.team, static_cast<int>(phaseIndex));
        phase.awayFatigueGain = fatigue_engine::phaseFatigueGain(awayState.team, static_cast<int>(phaseIndex));
        phase.injuryRisk = clampValue(0.05 + intensity * 0.04 +
                                          (phase.homeFatigueGain + phase.awayFatigueGain) / 160.0,
                                      0.05, 0.22);
        timeline.phases.push_back(phase);

        MatchEvent controlEvent;
        controlEvent.minute = minuteStart;
        controlEvent.teamName = phase.dominantTeam;
        controlEvent.type = MatchEventType::PossessionPhase;
        controlEvent.description = phase.dominantTeam + " domina el tramo " + to_string(minuteStart) + "-" + to_string(minuteEnd);
        timeline.events.push_back(controlEvent);

        homePossAccumulator += homePossessionShare;
        const int homeChanceCount = clampInt(static_cast<int>(round(homeOpportunity * (homePossessionShare >= 54 ? 5.2 : 3.8) * intensity +
                                                                   rand01() * 1.4 - 0.3)),
                                             0, 4);
        const int awayChanceCount = clampInt(static_cast<int>(round(awayOpportunity * (awayPossessionShare >= 54 ? 5.2 : 3.8) * intensity +
                                                                   rand01() * 1.4 - 0.3)),
                                             0, 4);

        auto playChances = [&](TeamRuntimeState& attacking,
                               TeamRuntimeState& defending,
                               const TeamMatchSnapshot& attackingSnapshot,
                               const TeamMatchSnapshot& defendingSnapshot,
                               bool attackingIsHome,
                               int chanceCount,
                               double attackingEdge) {
            for (int i = 0; i < chanceCount; ++i) {
                const int minute = randInt(minuteStart, minuteEnd);
                const bool bigChance = rand01() <= clampValue(0.12 + max(0.0, attackingEdge) * 0.02 +
                                                                  tactics_engine::transitionThreatWeight(attackingSnapshot.tacticalProfile) * 0.10,
                                                              0.10, 0.42);
                MatchEvent buildUp;
                buildUp.minute = max(minuteStart, minute - 1);
                buildUp.teamName = attacking.team.name;
                buildUp.type = (attacking.team.tactics == "Counter" || attacking.team.matchInstruction == "Juego directo")
                                   ? MatchEventType::Counterattack
                                   : MatchEventType::AttackBuildUp;
                buildUp.description = (buildUp.type == MatchEventType::Counterattack ? attacking.team.name + " acelera la transicion"
                                                                                      : attacking.team.name + " arma la jugada con paciencia");
                timeline.events.push_back(buildUp);

                ChanceResolutionInput input;
                input.minute = minute;
                input.bigChance = bigChance;
                input.attackingTeamIsHome = attackingIsHome;
                input.chanceQuality = clampValue(0.08 + attackingEdge * 0.012 + bigChance * 0.18 +
                                                     tactics_engine::transitionThreatWeight(attackingSnapshot.tacticalProfile) * 0.10 -
                                                     tactics_engine::defensiveSecurityWeight(defendingSnapshot.tacticalProfile) * 0.05,
                                                 0.06, 0.70);
                input.attackingEdge = attackingEdge;
                input.defensivePressure = max(0.10, defendingSnapshot.defensePower / 210.0);

                ChanceResolutionOutput output =
                    match_resolution::resolveChance(attacking.team, defending.team, attacking.xi, defending.xi, input);
                pushEvent(timeline, stats, output.attemptEvent);
                for (const MatchEvent& event : output.outcomeEvents) pushEvent(timeline, stats, event);
                if (bigChance) {
                    if (attackingIsHome) stats.homeBigChances++;
                    else stats.awayBigChances++;
                }
                if (output.scored && output.shooterIndex >= 0 &&
                    output.shooterIndex < static_cast<int>(attacking.team.players.size())) {
                    GoalContribution contribution;
                    contribution.minute = minute;
                    contribution.scorerName = attacking.team.players[static_cast<size_t>(output.shooterIndex)].name;
                    if (output.assisterIndex >= 0 &&
                        output.assisterIndex < static_cast<int>(attacking.team.players.size()) &&
                        output.assisterIndex != output.shooterIndex) {
                        contribution.assisterName = attacking.team.players[static_cast<size_t>(output.assisterIndex)].name;
                    }
                    attacking.goals.push_back(contribution);
                }
            }
        };

        playChances(homeState, awayState, homeSnapshot, awaySnapshot, true, homeChanceCount, homeAttack - awayDefense);
        playChances(awayState, homeState, awaySnapshot, homeSnapshot, false, awayChanceCount, awayAttack - homeDefense);

        auto registerDiscipline = [&](TeamRuntimeState& state, bool stateIsHome, vector<string>& yellows, vector<string>& reds) {
            MatchEvent foulEvent;
            foulEvent.minute = randInt(minuteStart, minuteEnd);
            foulEvent.teamName = state.team.name;
            foulEvent.type = MatchEventType::Foul;
            foulEvent.description = state.team.name + " corta el ritmo con una falta tactica";
            if (stateIsHome) foulEvent.impact.homeFoulsDelta = clampInt(static_cast<int>(round(intensity * (1.0 + state.team.pressingIntensity * 0.22))) + randInt(0, 2), 1, 5);
            else foulEvent.impact.awayFoulsDelta = clampInt(static_cast<int>(round(intensity * (1.0 + state.team.pressingIntensity * 0.22))) + randInt(0, 2), 1, 5);
            pushEvent(timeline, stats, foulEvent);

            if (rand01() <= clampValue(0.10 + intensity * 0.08 + max(0, state.team.pressingIntensity - 3) * 0.04, 0.05, 0.34) &&
                !state.xi.empty()) {
                const int idx = state.xi[static_cast<size_t>(randInt(0, static_cast<int>(state.xi.size()) - 1))];
                if (idx >= 0 && idx < static_cast<int>(state.team.players.size())) {
                    const string playerName = state.team.players[static_cast<size_t>(idx)].name;
                    state.cautionedPlayers.push_back(playerName);
                    yellows.push_back(playerName);
                    MatchEvent yellow;
                    yellow.minute = randInt(minuteStart, minuteEnd);
                    yellow.teamName = state.team.name;
                    yellow.playerName = playerName;
                    yellow.type = MatchEventType::YellowCard;
                    yellow.description = playerName + " ve la amarilla por llegar tarde";
                    if (stateIsHome) yellow.impact.homeYellowCardsDelta = 1;
                    else yellow.impact.awayYellowCardsDelta = 1;
                    pushEvent(timeline, stats, yellow);
                }
            }
        };

        registerDiscipline(homeState, true, data.homeYellowCardPlayers, data.homeRedCardPlayers);
        registerDiscipline(awayState, false, data.awayYellowCardPlayers, data.awayRedCardPlayers);

        auto maybeInjure = [&](TeamRuntimeState& state, vector<string>& injuredPlayers) {
            if (rand01() > phase.injuryRisk) return;
            const int idx = pickPhysicalRiskPlayer(state);
            if (idx < 0 || idx >= static_cast<int>(state.team.players.size())) return;
            const string playerName = state.team.players[static_cast<size_t>(idx)].name;
            if (find(injuredPlayers.begin(), injuredPlayers.end(), playerName) != injuredPlayers.end()) return;
            injuredPlayers.push_back(playerName);
            MatchEvent event;
            event.minute = randInt(minuteStart, minuteEnd);
            event.teamName = state.team.name;
            event.playerName = playerName;
            event.type = MatchEventType::Injury;
            event.description = playerName + " acusa un problema fisico por el desgaste";
            timeline.events.push_back(event);
        };

        maybeInjure(homeState, data.homeInjuredPlayers);
        maybeInjure(awayState, data.awayInjuredPlayers);
        fatigue_engine::applyPhaseFatigue(homeState.team, homeState.xi, static_cast<int>(phaseIndex));
        fatigue_engine::applyPhaseFatigue(awayState.team, awayState.xi, static_cast<int>(phaseIndex));
    }

    stats.homePossession = clampInt(static_cast<int>(round(homePossAccumulator / static_cast<double>(kPhases.size()))), 30, 70);
    stats.awayPossession = 100 - stats.homePossession;

    data.homeParticipants = homeState.participants;
    data.awayParticipants = awayState.participants;
    data.homeGoals = homeState.goals;
    data.awayGoals = awayState.goals;

    MatchResult result;
    result.homeGoals = stats.homeGoals;
    result.awayGoals = stats.awayGoals;
    result.homeShots = stats.homeShots;
    result.awayShots = stats.awayShots;
    result.homePossession = stats.homePossession;
    result.awayPossession = stats.awayPossession;
    result.homeSubstitutions = static_cast<int>(count_if(timeline.events.begin(), timeline.events.end(), [&](const MatchEvent& event) {
        return event.type == MatchEventType::Substitution && event.teamName == home.name;
    }));
    result.awaySubstitutions = static_cast<int>(count_if(timeline.events.begin(), timeline.events.end(), [&](const MatchEvent& event) {
        return event.type == MatchEventType::Substitution && event.teamName == away.name;
    }));
    result.homeCorners = stats.homeCorners;
    result.awayCorners = stats.awayCorners;
    result.weather = setup.context.weather;
    result.context = setup.context;
    result.stats = stats;
    result.timeline = timeline;
    result.events = buildLegacyTimeline(timeline);
    result.reportLines.push_back("--- Partido ---");
    result.reportLines.push_back(home.name + " vs " + away.name);
    result.reportLines.push_back("Clima: " + setup.context.weather + " | xG " + formatDouble2(stats.homeExpectedGoals) +
                                 " - " + formatDouble2(stats.awayExpectedGoals));
    result.reportLines.push_back("Resultado Final: " + home.name + " " + to_string(stats.homeGoals) + " - " +
                                 to_string(stats.awayGoals) + " " + away.name);
    result.reportLines.push_back("Estadisticas: Tiros " + to_string(stats.homeShots) + "-" + to_string(stats.awayShots) +
                                 ", Tiros al arco " + to_string(stats.homeShotsOnTarget) + "-" +
                                 to_string(stats.awayShotsOnTarget) +
                                 ", Posesion " + to_string(stats.homePossession) + "%-" +
                                 to_string(stats.awayPossession) + "%" +
                                 ", Corners " + to_string(stats.homeCorners) + "-" + to_string(stats.awayCorners));
    result.reportLines.push_back("Analisis probable: " + buildProbableReason(setup, stats, home, away));
    if (setup.context.fatigueFactorHome < 0.92 || setup.context.fatigueFactorAway < 0.92) {
        result.warnings.push_back("El desgaste acumulado tuvo impacto directo en el rendimiento.");
    }
    result.verdict = stats.homeGoals > stats.awayGoals ? "Victoria local"
                     : stats.homeGoals < stats.awayGoals ? "Victoria visitante"
                                                         : "Empate";
    data.result = std::move(result);
    return data;
}

}  // namespace match_engine
