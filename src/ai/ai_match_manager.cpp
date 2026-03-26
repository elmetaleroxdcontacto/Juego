#include "ai/ai_match_manager.h"

#include "ai/team_ai.h"
#include "simulation/fatigue_engine.h"
#include "simulation/match_engine_internal.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

bool isCautioned(const vector<string>& cautionedPlayers, const string& name) {
    return find(cautionedPlayers.begin(), cautionedPlayers.end(), name) != cautionedPlayers.end();
}

bool applyBenchSubstitution(Team& team,
                            vector<int>& xi,
                            vector<int>& participants,
                            const vector<string>& cautionedPlayers,
                            int minute,
                            MatchTimeline& timeline) {
    if (xi.empty()) return false;

    int playerOut = -1;
    int outSlot = -1;
    int highestNeed = 0;
    for (size_t slot = 0; slot < xi.size(); ++slot) {
        const int idx = xi[slot];
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[static_cast<size_t>(idx)];
        const int need = fatigue_engine::substitutionNeedScore(team, idx, isCautioned(cautionedPlayers, player.name));
        if (need > highestNeed) {
            highestNeed = need;
            playerOut = idx;
            outSlot = static_cast<int>(slot);
        }
    }
    if (playerOut < 0 || highestNeed < 10) return false;

    const string targetPos = normalizePosition(team.players[static_cast<size_t>(playerOut)].position);
    const int playerIn = match_internal::bestBenchReplacement(team, xi, targetPos);
    if (playerIn < 0) return false;

    xi[static_cast<size_t>(outSlot)] = playerIn;
    if (find(participants.begin(), participants.end(), playerIn) == participants.end()) {
        participants.push_back(playerIn);
    }

    MatchEvent event;
    event.minute = minute;
    event.teamName = team.name;
    event.playerName = team.players[static_cast<size_t>(playerIn)].name;
    event.type = MatchEventType::Substitution;
    event.description = team.players[static_cast<size_t>(playerOut)].name + " deja el campo por " +
                        team.players[static_cast<size_t>(playerIn)].name;
    timeline.events.push_back(event);
    return true;
}

int averageActiveFitness(const Team& team, const vector<int>& xi) {
    int total = 0;
    int count = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        total += team.players[static_cast<size_t>(idx)].fitness;
        count++;
    }
    return count > 0 ? total / count : 50;
}

}  // namespace

namespace ai_match_manager {

bool applyInMatchManagement(Team& team,
                            const Team& opponent,
                            vector<int>& xi,
                            vector<int>& participants,
                            const vector<string>& cautionedPlayers,
                            int minute,
                            int goalsFor,
                            int goalsAgainst,
                            int opponentAvailablePlayers,
                            MatchTimeline& timeline) {
    bool changed = false;
    vector<string> notes;
    if (team_ai::applyInMatchCpuAdjustment(team,
                                           opponent,
                                           minute,
                                           goalsFor,
                                           goalsAgainst,
                                           &notes,
                                           static_cast<int>(xi.size()),
                                           static_cast<int>(cautionedPlayers.size()),
                                           opponentAvailablePlayers)) {
        changed = true;
        for (const string& note : notes) {
            MatchEvent event;
            event.minute = minute;
            event.teamName = team.name;
            event.type = MatchEventType::TacticalChange;
            event.description = note;
            timeline.events.push_back(event);
        }
    }

    if (minute >= 55 && applyBenchSubstitution(team, xi, participants, cautionedPlayers, minute, timeline)) {
        changed = true;
    }
    if (minute >= 72 && averageActiveFitness(team, xi) < 56 &&
        applyBenchSubstitution(team, xi, participants, cautionedPlayers, minute + 1, timeline)) {
        changed = true;
    }
    return changed;
}

}  // namespace ai_match_manager
