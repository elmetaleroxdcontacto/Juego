#include "career/dressing_room_service.h"

#include "career/team_management.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

int expectedStartsForPromise(const Player& player, int currentWeek) {
    if (player.promisedRole == "Titular") return max(2, currentWeek * 2 / 3);
    if (player.promisedRole == "Rotacion") return max(1, currentWeek / 3);
    if (player.promisedRole == "Proyecto") {
        return (player.age <= 22) ? max(1, currentWeek / 4) : max(0, currentWeek / 6);
    }
    return max(0, currentWeek / 2 + player.desiredStarts - 1);
}

bool isLeader(const Player& player) {
    return player.leadership >= 72 || playerHasTrait(player, "Lider");
}

string buildSummary(const DressingRoomSnapshot& snapshot) {
    ostringstream out;
    out << snapshot.climate << " | promesas en riesgo " << snapshot.promiseRiskCount
        << " | moral baja " << snapshot.lowMoraleCount
        << " | fatiga alta " << snapshot.fatigueRiskCount
        << " | salidas " << snapshot.wantsOutCount;
    return out.str();
}

}  // namespace

namespace dressing_room_service {

DressingRoomSnapshot buildSnapshot(const Team& team, int currentWeek) {
    DressingRoomSnapshot snapshot;
    snapshot.climate = dressingRoomClimate(team);

    for (const Player& player : team.players) {
        if (isLeader(player) && player.happiness >= 55) snapshot.leaderCount++;
        if (promiseAtRisk(player, currentWeek)) {
            snapshot.promiseRiskCount++;
            if (snapshot.alerts.size() < 8) {
                snapshot.alerts.push_back(player.name + " siente en riesgo su promesa de rol (" + player.promisedRole + ").");
            }
        }
        if (player.happiness < 45) {
            snapshot.lowMoraleCount++;
            if (snapshot.alerts.size() < 8) {
                snapshot.alerts.push_back(player.name + " cae a moral " + to_string(player.happiness) + ".");
            }
        }
        if (player.fitness < 58) {
            snapshot.fatigueRiskCount++;
            if (snapshot.alerts.size() < 8) {
                snapshot.alerts.push_back(player.name + " llega al limite fisico con " + to_string(player.fitness) + " de condicion.");
            }
        }
        if (player.wantsToLeave) {
            snapshot.wantsOutCount++;
            if (snapshot.alerts.size() < 8) {
                snapshot.alerts.push_back(player.name + " evalua salir del club.");
            }
        }
    }

    if (snapshot.alerts.empty()) {
        snapshot.alerts.push_back("El vestuario no presenta focos criticos esta semana.");
    }
    snapshot.summary = buildSummary(snapshot);
    return snapshot;
}

DressingRoomSnapshot applyWeeklyUpdate(Career& career, int pointsDelta) {
    DressingRoomSnapshot empty;
    if (!career.myTeam) return empty;

    Team& team = *career.myTeam;
    const int squadAverage = team.getAverageSkill();
    int captainLeadership = 0;
    const int captainIndex = team_mgmt::playerIndexByName(team, team.captain);
    if (captainIndex >= 0 && captainIndex < static_cast<int>(team.players.size())) {
        captainLeadership = team.players[static_cast<size_t>(captainIndex)].leadership;
    }

    for (Player& player : team.players) {
        const bool wasUnhappy = player.wantsToLeave;
        const int expectedStarts = expectedStartsForPromise(player, career.currentWeek);
        const int unmetStarts = max(0, expectedStarts - player.startsThisSeason);

        if (unmetStarts > 0) {
            player.happiness = clampInt(player.happiness - min(4, 1 + unmetStarts), 1, 99);
            if (player.skill >= squadAverage && unmetStarts >= 2) {
                player.chemistry = clampInt(player.chemistry - 1, 1, 99);
            }
            if (player.promisedRole != "Sin promesa" && unmetStarts >= 2) {
                player.happiness = clampInt(player.happiness - 2, 1, 99);
            }
        } else {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            if (player.promisedRole != "Sin promesa") {
                player.happiness = clampInt(player.happiness + 1, 1, 99);
            }
        }

        if (pointsDelta == 3) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        } else if (pointsDelta == 0 && player.ambition >= 65) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.injured) player.happiness = clampInt(player.happiness - 1, 1, 99);
        if (player.fitness < 58) {
            player.happiness = clampInt(player.happiness - (player.fitness < 52 ? 2 : 1), 1, 99);
            if (team.pressingIntensity >= 4) {
                player.chemistry = clampInt(player.chemistry - 1, 1, 99);
            }
        }
        if (player.contractWeeks > 0 && player.contractWeeks <= 12 && unmetStarts > 0) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.wage < wageDemandFor(player) * 80 / 100) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (captainLeadership >= 75) player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        if (team.morale >= 65) player.happiness = clampInt(player.happiness + 1, 1, 99);
        if (playerHasTrait(player, "Lider") && team.morale >= 55) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (playerHasTrait(player, "Caliente") && pointsDelta == 0) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.leadership >= 75 && player.happiness >= 60) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (player.professionalism >= 75 && player.startsThisSeason >= expectedStarts) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
        }

        int unrestThreshold = (player.skill >= squadAverage + 3) ? 38 : 32;
        if (player.fitness < 55) unrestThreshold += 2;
        if (player.contractWeeks > 0 && player.contractWeeks <= 12) unrestThreshold += 1;
        player.wantsToLeave = player.happiness <= unrestThreshold &&
                              (player.ambition >= 60 || player.professionalism <= 45 || unmetStarts >= 3 ||
                               player.promisedRole != "Sin promesa");
        if (!wasUnhappy && player.wantsToLeave) {
            career.addNews(player.name + " queda disconforme con su situacion en " + team.name + ".");
        }
    }

    DressingRoomSnapshot snapshot = buildSnapshot(team, career.currentWeek);
    if (snapshot.promiseRiskCount >= 2 || snapshot.wantsOutCount > 0 || snapshot.lowMoraleCount >= 3) {
        career.addNews("Vestuario: " + snapshot.summary + ".");
    }
    return snapshot;
}

string formatSnapshot(const Career& career, size_t maxAlerts) {
    if (!career.myTeam) return "No hay informe de vestuario.";
    DressingRoomSnapshot snapshot = buildSnapshot(*career.myTeam, career.currentWeek);
    ostringstream out;
    out << "DressingRoomPanel\r\n" << snapshot.summary << "\r\n";
    const size_t count = min(maxAlerts, snapshot.alerts.size());
    for (size_t i = 0; i < count; ++i) {
        out << "- " << snapshot.alerts[i] << "\r\n";
    }
    return out.str();
}

}  // namespace dressing_room_service
