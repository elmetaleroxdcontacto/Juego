#include "simulation/match_postprocess.h"

#include "development/player_progression_system.h"
#include "simulation/match_engine_internal.h"
#include "simulation/morale_engine.h"
#include "simulation/player_condition.h"
#include "simulation/simulation.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

void registerNamedCards(Team& team,
                        const vector<string>& yellowNames,
                        const vector<string>& redNames,
                        vector<string>* events) {
    for (const string& name : yellowNames) {
        for (auto& player : team.players) {
            if (player.name != name) continue;
            player.seasonYellowCards++;
            player.yellowAccumulation++;
            if (player.yellowAccumulation >= 5) {
                player.yellowAccumulation -= 5;
                player.matchesSuspended++;
                if (events) {
                    events->push_back("Sancion por acumulacion: " + player.name);
                }
            }
            break;
        }
    }

    for (const string& name : redNames) {
        for (auto& player : team.players) {
            if (player.name != name) continue;
            player.seasonRedCards++;
            player.matchesSuspended++;
            break;
        }
    }
}

int applyGoalContributions(Team& team,
                           const vector<GoalContribution>& contributions,
                           vector<string>* events) {
    int applied = 0;
    for (const auto& contribution : contributions) {
        bool scorerApplied = false;
        for (auto& player : team.players) {
            if (player.name != contribution.scorerName) continue;
            player.goals++;
            scorerApplied = true;
            applied++;
            break;
        }
        if (!scorerApplied) continue;
        if (!contribution.assisterName.empty() && contribution.assisterName != contribution.scorerName) {
            for (auto& player : team.players) {
                if (player.name == contribution.assisterName) {
                    player.assists++;
                    break;
                }
            }
        }
        if (events) {
            string text = to_string(contribution.minute) + "' Gol confirmado: " + contribution.scorerName;
            if (!contribution.assisterName.empty() && contribution.assisterName != contribution.scorerName) {
                text += " (asist: " + contribution.assisterName + ")";
            }
            events->push_back(text);
        }
    }
    return applied;
}

void applyNamedInjuries(Team& team, const vector<string>& injuredNames) {
    for (const string& name : injuredNames) {
        for (auto& player : team.players) {
            if (player.name != name || player.injured) continue;
            player_condition::applyInjury(player, team, 1);
            break;
        }
    }
}

void applyDevelopment(Team& team, const vector<int>& xi, vector<string>* events) {
    development::applyMatchExperience(team, xi, events);
}

void updateMatchForm(Team& team,
                     const vector<int>& participants,
                     int goalsFor,
                     int goalsAgainst,
                     bool keyMatch) {
    int baseDelta = 0;
    if (goalsFor > goalsAgainst) baseDelta = 3;
    else if (goalsFor < goalsAgainst) baseDelta = -3;
    if (keyMatch) baseDelta += (baseDelta >= 0) ? 1 : -1;
    for (int idx : participants) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[static_cast<size_t>(idx)];
        int delta = baseDelta;
        const string pos = normalizePosition(player.position);
        if (pos == "DEL" && goalsFor >= 2) delta += 1;
        if ((pos == "DEF" || pos == "ARQ") && goalsAgainst == 0) delta += 1;
        if (player.fitness < 50) delta -= 1;
        if (player.injured) delta -= 2;
        if (keyMatch && playerHasTrait(player, "Cita grande")) delta += 1;
        if (keyMatch && player.bigMatches <= 40) delta -= 1;
        player.currentForm = clampInt(player.currentForm + delta, 1, 99);
        player.moraleMomentum = clampInt(player.moraleMomentum + delta, -25, 25);
    }
}

void applyParticipantLoad(Team& team, const vector<int>& participants, bool won, bool keyMatch) {
    for (int idx : participants) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[static_cast<size_t>(idx)];
        int load = 8 + max(0, 100 - player.fitness) / 8;
        if (team.tactics == "Pressing") load += 4;
        if (team.tempo >= 4) load += 2;
        if (keyMatch) load += 2;
        if (player.position == player.promisedPosition) player.happiness = clampInt(player.happiness + 1, 1, 99);
        else if (!player.promisedPosition.empty() && player.promisedPosition != "N/A") {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        player.fatigueLoad = clampInt(player.fatigueLoad + load, 0, 100);
        if (won && player.leadership >= 70) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
    }
}

void applyCompetitionOutcome(Team& home, Team& away, const MatchResult& result) {
    home.goalsFor += result.homeGoals;
    home.goalsAgainst += result.awayGoals;
    away.goalsFor += result.awayGoals;
    away.goalsAgainst += result.homeGoals;
    away.awayGoals += result.awayGoals;
    home.yellowCards += result.stats.homeYellowCards;
    away.yellowCards += result.stats.awayYellowCards;
    home.redCards += result.stats.homeRedCards;
    away.redCards += result.stats.awayRedCards;

    if (result.homeGoals > result.awayGoals) {
        home.points += 3;
        home.wins++;
        away.losses++;
        home.addHeadToHeadPoints(away.name, 3);
        away.addHeadToHeadPoints(home.name, 0);
    } else if (result.homeGoals < result.awayGoals) {
        away.points += 3;
        away.wins++;
        home.losses++;
        home.addHeadToHeadPoints(away.name, 0);
        away.addHeadToHeadPoints(home.name, 3);
    } else {
        home.points += 1;
        away.points += 1;
        home.draws++;
        away.draws++;
        home.addHeadToHeadPoints(away.name, 1);
        away.addHeadToHeadPoints(home.name, 1);
    }
}

}  // namespace

namespace match_postprocess {

void applySimulationOutcome(Team& home,
                            Team& away,
                            const match_engine::MatchSimulationData& simulation,
                            const vector<int>& homeStartXI,
                            const vector<int>& awayStartXI,
                            bool keyMatch) {
    const MatchResult& result = simulation.result;
    applyCompetitionOutcome(home, away, result);

    home.morale = clampInt(home.morale + morale_engine::postMatchMoraleDelta(home, result.homeGoals, result.awayGoals, keyMatch), 0, 100);
    away.morale = clampInt(away.morale + morale_engine::postMatchMoraleDelta(away, result.awayGoals, result.homeGoals, keyMatch), 0, 100);

    const vector<int>& homeParticipants = simulation.homeParticipants.empty() ? homeStartXI : simulation.homeParticipants;
    const vector<int>& awayParticipants = simulation.awayParticipants.empty() ? awayStartXI : simulation.awayParticipants;
    for (int idx : homeParticipants) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) home.players[static_cast<size_t>(idx)].matchesPlayed++;
    }
    for (int idx : awayParticipants) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) away.players[static_cast<size_t>(idx)].matchesPlayed++;
    }

    registerNamedCards(home, simulation.homeYellowCardPlayers, simulation.homeRedCardPlayers, nullptr);
    registerNamedCards(away, simulation.awayYellowCardPlayers, simulation.awayRedCardPlayers, nullptr);

    const int appliedHomeGoals = applyGoalContributions(home, simulation.homeGoals, nullptr);
    const int appliedAwayGoals = applyGoalContributions(away, simulation.awayGoals, nullptr);
    if (appliedHomeGoals < result.homeGoals) {
        assignGoalsAndAssists(home, result.homeGoals - appliedHomeGoals, homeStartXI, home.name, nullptr);
    }
    if (appliedAwayGoals < result.awayGoals) {
        assignGoalsAndAssists(away, result.awayGoals - appliedAwayGoals, awayStartXI, away.name, nullptr);
    }

    applyNamedInjuries(home, simulation.homeInjuredPlayers);
    applyNamedInjuries(away, simulation.awayInjuredPlayers);
    applyDevelopment(home, homeParticipants, nullptr);
    applyDevelopment(away, awayParticipants, nullptr);
    updateMatchForm(home, homeParticipants, result.homeGoals, result.awayGoals, keyMatch);
    updateMatchForm(away, awayParticipants, result.awayGoals, result.homeGoals, keyMatch);
    applyParticipantLoad(home, homeParticipants, result.homeGoals > result.awayGoals, keyMatch);
    applyParticipantLoad(away, awayParticipants, result.awayGoals > result.homeGoals, keyMatch);
    match_internal::applyMatchFatigue(home, homeParticipants, home.tactics);
    match_internal::applyMatchFatigue(away, awayParticipants, away.tactics);
}

}  // namespace match_postprocess

int teamPenaltyStrength(const Team& team) {
    const vector<int> xi = team.getStartingXIIndices();
    int total = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[static_cast<size_t>(idx)];
        total += player.skill;
        total += player.setPieceSkill / 4;
        total += player.professionalism / 10;
        if (!team.penaltyTaker.empty() && player.name == team.penaltyTaker) total += 12 + player.setPieceSkill / 5;
        if (!team.captain.empty() && player.name == team.captain) total += 4 + player.leadership / 6;
    }
    if (xi.empty()) return 50;
    return total / static_cast<int>(xi.size());
}
