#include "simulation/match_event_generator.h"

#include "simulation/match_event_resolver.h"
#include "simulation/match_resolution.h"
#include "simulation/tactics_engine.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {

int pickPhysicalRiskPlayer(const Team& team, const vector<int>& activeXI) {
    int bestIndex = -1;
    int bestScore = -1;
    for (int idx : activeXI) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[static_cast<size_t>(idx)];
        int score = (100 - player.fitness) * 2 + max(0, player.age - 29) * 3 + player.injuryHistory * 4;
        if (playerHasTrait(player, "Fragil")) score += 10;
        if (score > bestScore) {
            bestScore = score;
            bestIndex = idx;
        }
    }
    return bestIndex;
}

void removeActivePlayer(vector<int>& activeXI, int playerIndex) {
    activeXI.erase(remove(activeXI.begin(), activeXI.end(), playerIndex), activeXI.end());
}

}  // namespace

namespace match_event_generator {

void playPhaseSequences(Team& attacking,
                        Team& defending,
                        const vector<int>& attackingXI,
                        const vector<int>& defendingXI,
                        const TeamMatchSnapshot& attackingSnapshot,
                        const TeamMatchSnapshot& defendingSnapshot,
                        bool attackingIsHome,
                        int minuteStart,
                        int minuteEnd,
                        int possessionChains,
                        int progressionCount,
                        int attackCount,
                        int chanceCount,
                        double attackingEdge,
                        double defensiveRisk,
                        MatchTimeline& timeline,
                        MatchStats& stats,
                        vector<GoalContribution>& goals) {
    if (attackCount <= 0 && chanceCount <= 0) return;

    const double transitionThreat = tactics_engine::transitionThreatWeight(attackingSnapshot.tacticalProfile);
    const double attackingSecurity = tactics_engine::defensiveSecurityWeight(attackingSnapshot.tacticalProfile);
    const double progressionSuccess = clampValue(0.28 + attackingSnapshot.pressResistance / 210.0 +
                                                     attackingSnapshot.chanceCreation / 260.0 +
                                                     attackingSnapshot.tacticalProfile.width * 0.06 +
                                                     possessionChains * 0.010 + attackingEdge * 0.0015 -
                                                     defendingSnapshot.pressingLoad / 420.0 +
                                                     defensiveRisk * 0.08,
                                                 0.16, 0.82);
    const double chanceReachProbability = clampValue(
        0.08 + static_cast<double>(chanceCount + 1) / static_cast<double>(max(1, attackCount + 3)) +
            attackingEdge * 0.0025 + transitionThreat * 0.10 + defensiveRisk * 0.08 + attackingSecurity * 0.03 +
            attackingSnapshot.finishingQuality / 420.0 - defendingSnapshot.defensiveShape / 620.0,
        0.06, 0.66);

    for (int i = 0; i < attackCount; ++i) {
        const int minute = randInt(minuteStart, minuteEnd);
        const bool directTransition = attacking.tactics == "Counter" ||
                                      attacking.matchInstruction == "Juego directo" ||
                                      rand01() <= clampValue(0.18 + transitionThreat * 0.45, 0.10, 0.62);

        if (i < progressionCount && rand01() <= progressionSuccess) {
            MatchEvent progression;
            progression.minute = max(minuteStart, minute - 2);
            progression.teamName = attacking.name;
            progression.type = MatchEventType::Progression;
            progression.description = directTransition ? attacking.name + " gana metros con una ruptura directa"
                                                       : attacking.name + " progresa entre lineas";
            match_stats::pushEvent(timeline, stats, progression);
        }

        MatchEvent buildUp;
        buildUp.minute = max(minuteStart, minute - 1);
        buildUp.teamName = attacking.name;
        buildUp.type = directTransition ? MatchEventType::Counterattack : MatchEventType::AttackBuildUp;
        buildUp.description = directTransition ? attacking.name + " acelera la transicion"
                                               : attacking.name + " madura el ataque en campo rival";
        match_stats::pushEvent(timeline, stats, buildUp);

        if (rand01() > chanceReachProbability) {
            MatchEvent interruption;
            interruption.minute = minute;
            interruption.teamName = rand01() <= 0.55 ? defending.name : attacking.name;
            const double interruptionRoll = rand01();
            if (interruptionRoll <= 0.38) {
                interruption.type = MatchEventType::Offside;
                interruption.description = attacking.name + " rompe mal la linea y queda adelantado";
            } else if (interruptionRoll <= 0.72) {
                interruption.type = MatchEventType::Foul;
                interruption.description = defending.name + " frena la jugada antes de que llegue al area";
            } else {
                interruption.type = MatchEventType::Progression;
                interruption.description = defending.name + " recupera y obliga a reiniciar la posesion";
            }
            match_stats::pushEvent(timeline, stats, interruption);
            continue;
        }

        const bool bigChance =
            rand01() <= clampValue(0.10 + max(0.0, attackingEdge) * 0.018 + transitionThreat * 0.12 -
                                       defensiveRisk * 0.05,
                                   0.10, 0.42);

        ChanceResolutionInput input;
        input.minute = minute;
        input.bigChance = bigChance;
        input.attackingTeamIsHome = attackingIsHome;
        input.chanceQuality = clampValue(0.06 + attackingEdge * 0.010 + bigChance * 0.18 +
                                             transitionThreat * 0.12 -
                                             tactics_engine::defensiveSecurityWeight(defendingSnapshot.tacticalProfile) * 0.05 +
                                             attackingSnapshot.finishingQuality / 500.0 -
                                             defendingSnapshot.goalkeeperPower / 950.0,
                                         0.05, 0.52);
        input.attackingEdge = attackingEdge;
        input.defensivePressure = max(0.14, defendingSnapshot.defensePower / 185.0 + defendingSnapshot.pressingLoad / 520.0);
        input.finishingSupport = attackingSnapshot.finishingQuality / 100.0;
        input.goalkeeperQuality = defendingSnapshot.goalkeeperPower / 100.0;

        ChanceResolutionOutput output =
            match_event_resolver::resolveChance(attacking, defending, attackingXI, defendingXI, input);
        match_stats::pushEvent(timeline, stats, output.attemptEvent);
        for (const MatchEvent& event : output.outcomeEvents) {
            match_stats::pushEvent(timeline, stats, event);
        }
        if (bigChance) {
            if (attackingIsHome) stats.homeBigChances++;
            else stats.awayBigChances++;
        }
        if (output.scored && output.shooterIndex >= 0 && output.shooterIndex < static_cast<int>(attacking.players.size())) {
            GoalContribution contribution;
            contribution.minute = minute;
            contribution.scorerName = attacking.players[static_cast<size_t>(output.shooterIndex)].name;
            if (output.assisterIndex >= 0 && output.assisterIndex < static_cast<int>(attacking.players.size()) &&
                output.assisterIndex != output.shooterIndex) {
                contribution.assisterName = attacking.players[static_cast<size_t>(output.assisterIndex)].name;
            }
            goals.push_back(contribution);
        }
    }
}

void registerDiscipline(Team& team,
                        vector<int>& activeXI,
                        bool teamIsHome,
                        double intensity,
                        MatchTimeline& timeline,
                        MatchStats& stats,
                        vector<string>& cautionedPlayers,
                        vector<string>& sentOffPlayers,
                        vector<string>& yellowCardPlayers,
                        vector<string>& redCardPlayers) {
    MatchEvent foulEvent;
    foulEvent.minute = randInt(timeline.phases.back().minuteStart, timeline.phases.back().minuteEnd);
    foulEvent.teamName = team.name;
    foulEvent.type = MatchEventType::Foul;
    foulEvent.description = team.name + " corta el ritmo con una falta tactica";
    int fouls = clampInt(static_cast<int>(round(intensity * (1.0 + team.pressingIntensity * 0.22))) + randInt(0, 2), 1, 5);
    if (teamIsHome) foulEvent.impact.homeFoulsDelta = fouls;
    else foulEvent.impact.awayFoulsDelta = fouls;
    match_stats::pushEvent(timeline, stats, foulEvent);

    if (activeXI.empty()) return;
    double cautionProbability = clampValue(0.10 + intensity * 0.08 + max(0, team.pressingIntensity - 3) * 0.04, 0.05, 0.34);
    if (rand01() > cautionProbability) return;

    const int idx = activeXI[static_cast<size_t>(randInt(0, static_cast<int>(activeXI.size()) - 1))];
    if (idx < 0 || idx >= static_cast<int>(team.players.size())) return;
    const string playerName = team.players[static_cast<size_t>(idx)].name;
    const bool alreadyBooked = find(cautionedPlayers.begin(), cautionedPlayers.end(), playerName) != cautionedPlayers.end();
    const bool straightRed = rand01() <= clampValue(0.02 + max(0, team.pressingIntensity - 3) * 0.02 + intensity * 0.02, 0.02, 0.10);

    if (alreadyBooked || straightRed) {
        sentOffPlayers.push_back(playerName);
        redCardPlayers.push_back(playerName);
        removeActivePlayer(activeXI, idx);

        MatchEvent red;
        red.minute = randInt(timeline.phases.back().minuteStart, timeline.phases.back().minuteEnd);
        red.teamName = team.name;
        red.playerName = playerName;
        red.type = MatchEventType::RedCard;
        red.description = straightRed ? playerName + " ve la roja directa por una entrada temeraria"
                                      : playerName + " es expulsado por doble amarilla";
        if (teamIsHome) red.impact.homeRedCardsDelta = 1;
        else red.impact.awayRedCardsDelta = 1;
        match_stats::pushEvent(timeline, stats, red);
        return;
    }

    cautionedPlayers.push_back(playerName);
    yellowCardPlayers.push_back(playerName);
    MatchEvent yellow;
    yellow.minute = randInt(timeline.phases.back().minuteStart, timeline.phases.back().minuteEnd);
    yellow.teamName = team.name;
    yellow.playerName = playerName;
    yellow.type = MatchEventType::YellowCard;
    yellow.description = playerName + " ve la amarilla por llegar tarde";
    if (teamIsHome) yellow.impact.homeYellowCardsDelta = 1;
    else yellow.impact.awayYellowCardsDelta = 1;
    match_stats::pushEvent(timeline, stats, yellow);
}

void maybeInjure(const Team& team,
                 const vector<int>& activeXI,
                 double injuryRisk,
                 int minuteStart,
                 int minuteEnd,
                 MatchTimeline& timeline,
                 vector<string>& injuredPlayers) {
    if (rand01() > injuryRisk) return;
    const int idx = pickPhysicalRiskPlayer(team, activeXI);
    if (idx < 0 || idx >= static_cast<int>(team.players.size())) return;
    const string playerName = team.players[static_cast<size_t>(idx)].name;
    if (find(injuredPlayers.begin(), injuredPlayers.end(), playerName) != injuredPlayers.end()) return;

    injuredPlayers.push_back(playerName);
    MatchEvent event;
    event.minute = randInt(minuteStart, minuteEnd);
    event.teamName = team.name;
    event.playerName = playerName;
    event.type = MatchEventType::Injury;
    event.description = playerName + " acusa un problema fisico por el desgaste";
    timeline.events.push_back(event);
}

}  // namespace match_event_generator
