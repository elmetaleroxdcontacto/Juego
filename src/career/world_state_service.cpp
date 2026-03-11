#include "career/world_state_service.h"

#include "career/career_reports.h"
#include "competition/competition.h"
#include "utils/utils.h"

#include <algorithm>
#include <sstream>

using namespace std;

namespace {

int targetStartsForPromise(const string& target) {
    if (target == "Titular") return 4;
    if (target == "Rotacion") return 2;
    if (target == "Proyecto") return 1;
    return 2;
}

Player* findPlayerByName(Team& team, const string& name) {
    for (auto& player : team.players) {
        if (player.name == name) return &player;
    }
    return nullptr;
}

const Player* bestYouthProspect(const Team& team) {
    const Player* best = nullptr;
    int bestGap = -1;
    for (const auto& player : team.players) {
        if (player.age > 21) continue;
        const int gap = player.potential - player.skill + max(0, 22 - player.age);
        if (gap > bestGap) {
            bestGap = gap;
            best = &player;
        }
    }
    return best;
}

const Player* minutesPromiseCandidate(const Team& team) {
    const Player* promised = nullptr;
    int promisedScore = -1000;
    for (const auto& player : team.players) {
        if (player.promisedRole == "Sin promesa") continue;
        const int score = player.desiredStarts * 3 + player.skill + max(0, player.potential - player.skill);
        if (score > promisedScore) {
            promisedScore = score;
            promised = &player;
        }
    }
    return promised ? promised : bestYouthProspect(team);
}

int qualityPlayersAtPosition(const Team& team, const string& position) {
    const int threshold = team.getAverageSkill() - 4;
    int count = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != normalizePosition(position)) continue;
        if (player.skill >= threshold) count++;
    }
    return count;
}

void pushHeadline(WorldPulseSummary& summary, const string& line) {
    if (line.empty()) return;
    if (summary.headlines.size() < 8) summary.headlines.push_back(line);
}

bool updateRecord(vector<HistoricalRecord>& records,
                  const string& category,
                  const string& holderName,
                  const string& teamName,
                  int season,
                  int value,
                  const string& note,
                  bool lowerIsBetter) {
    auto it = find_if(records.begin(), records.end(), [&](const HistoricalRecord& record) {
        return record.category == category;
    });
    const bool better = (it == records.end())
                            ? true
                            : (lowerIsBetter ? value < it->value
                                             : value > it->value);
    if (!better) return false;

    HistoricalRecord record;
    record.category = category;
    record.holderName = holderName;
    record.teamName = teamName;
    record.season = season;
    record.value = value;
    record.note = note;
    if (it == records.end()) records.push_back(record);
    else *it = record;
    return true;
}

string promiseLabel(const SquadPromise& promise) {
    ostringstream out;
    out << promise.category << " | " << promise.subjectName << " | " << promise.target
        << " | progreso " << promise.progress << " | vence F" << promise.deadlineWeek;
    return out.str();
}

}  // namespace

namespace world_state_service {

void seedSeasonPromises(Career& career) {
    career.activePromises.clear();
    if (!career.myTeam) return;

    const int seasonDeadline = max(career.currentWeek + 4, static_cast<int>(career.schedule.size()));
    const int competitionTarget =
        max(1, career.boardExpectedFinish > 0 ? career.boardExpectedFinish
                                              : max(1, career.currentCompetitiveFieldSize() / 3));
    career.activePromises.push_back({
        career.myTeam->name,
        "Competicion",
        to_string(competitionTarget),
        career.currentWeek,
        seasonDeadline,
        0,
        false,
        false,
    });

    const string need = detectScoutingNeed(*career.myTeam);
    if (qualityPlayersAtPosition(*career.myTeam, need) < 3) {
        career.activePromises.push_back({
            career.myTeam->name,
            "Fichaje",
            need,
            career.currentWeek,
            min(seasonDeadline, career.currentWeek + 6),
            0,
            false,
            false,
        });
    }

    const Player* youth = minutesPromiseCandidate(*career.myTeam);
    if (youth) {
        const string target = youth->promisedRole != "Sin promesa" ? youth->promisedRole : "Proyecto";
        career.activePromises.push_back({
            youth->name,
            "Minutos",
            target,
            career.currentWeek,
            min(seasonDeadline, career.currentWeek + 8),
            0,
            false,
            false,
        });
    }
}

WorldPulseSummary processWeeklyWorldState(Career& career) {
    WorldPulseSummary summary;
    if (!career.myTeam) return summary;

    Team& team = *career.myTeam;
    for (size_t i = 0; i < career.activePromises.size();) {
        SquadPromise& promise = career.activePromises[i];
        bool resolved = false;

        if (promise.category == "Minutos") {
            Player* player = findPlayerByName(team, promise.subjectName);
            if (!player) {
                promise.failed = true;
                resolved = true;
            } else {
                promise.progress = player->startsThisSeason;
                const int targetStarts = targetStartsForPromise(promise.target);
                if (promise.progress >= targetStarts) {
                    promise.fulfilled = true;
                    player->happiness = clampInt(player->happiness + 5, 1, 99);
                    player->moraleMomentum = clampInt(player->moraleMomentum + 3, -25, 25);
                    team.morale = clampInt(team.morale + 2, 0, 100);
                    career.addNews("Promesa cumplida: " + player->name + " recibe los minutos prometidos.");
                    pushHeadline(summary, "Promesa cumplida: " + player->name + " consolida su rol en el plantel.");
                    summary.resolvedPromises++;
                    resolved = true;
                } else if (career.currentWeek >= promise.deadlineWeek) {
                    promise.failed = true;
                    player->happiness = clampInt(player->happiness - 8, 1, 99);
                    player->moraleMomentum = clampInt(player->moraleMomentum - 4, -25, 25);
                    player->wantsToLeave = player->ambition >= 58 || player->promisedRole != "Sin promesa";
                    team.morale = clampInt(team.morale - 2, 0, 100);
                    career.addNews("Promesa rota: " + player->name + " no recibe los minutos comprometidos.");
                    pushHeadline(summary, "Vestuario: " + player->name + " queda molesto por una promesa rota.");
                    summary.brokenPromises++;
                    resolved = true;
                }
            }
        } else if (promise.category == "Fichaje") {
            promise.progress = qualityPlayersAtPosition(team, promise.target);
            if (promise.progress >= 3) {
                promise.fulfilled = true;
                career.boardConfidence = clampInt(career.boardConfidence + 2, 0, 100);
                career.addNews("Promesa cumplida: el plantel cubre la necesidad en " + promise.target + ".");
                pushHeadline(summary, "Plantel reforzado: el club ya tiene profundidad suficiente en " + promise.target + ".");
                summary.resolvedPromises++;
                resolved = true;
            } else if (career.currentWeek >= promise.deadlineWeek) {
                promise.failed = true;
                career.boardConfidence = clampInt(career.boardConfidence - 3, 0, 100);
                team.morale = clampInt(team.morale - 1, 0, 100);
                career.addNews("Promesa rota: no llega el refuerzo esperado para la zona " + promise.target + ".");
                pushHeadline(summary, "Mercado incompleto: sigue faltando profundidad en " + promise.target + ".");
                summary.brokenPromises++;
                resolved = true;
            }
        } else if (promise.category == "Competicion") {
            int targetRank = 1;
            try {
                targetRank = max(1, stoi(promise.target));
            } catch (...) {
                targetRank = 1;
            }
            const int currentRank = max(1, career.currentCompetitiveRank());
            promise.progress = max(0, targetRank - currentRank + 1);
            if (career.currentWeek >= promise.deadlineWeek) {
                if (currentRank <= targetRank) {
                    promise.fulfilled = true;
                    career.managerReputation = clampInt(career.managerReputation + 2, 1, 100);
                    team.morale = clampInt(team.morale + 3, 0, 100);
                    career.addNews("Promesa cumplida: " + team.name + " sostiene el objetivo competitivo del curso.");
                    pushHeadline(summary, "Proyecto deportivo: " + team.name + " sostiene la meta competitiva.");
                    summary.resolvedPromises++;
                } else {
                    promise.failed = true;
                    career.managerReputation = clampInt(career.managerReputation - 2, 1, 100);
                    career.boardConfidence = clampInt(career.boardConfidence - 4, 0, 100);
                    team.morale = clampInt(team.morale - 3, 0, 100);
                    career.addNews("Promesa rota: " + team.name + " no alcanza el objetivo de tabla comprometido.");
                    pushHeadline(summary, "Directiva: aumenta la presion tras incumplir la meta de tabla.");
                    summary.brokenPromises++;
                }
                resolved = true;
            }
        }

        if (resolved) {
            career.activePromises.erase(career.activePromises.begin() + static_cast<long long>(i));
            continue;
        }
        if (promise.deadlineWeek - career.currentWeek <= 1) {
            pushHeadline(summary, "Promesa bajo presion: " + promiseLabel(promise));
        }
        ++i;
    }

    int pressureClubs = 0;
    const Team* mostPressured = nullptr;
    int worstScore = 1000000;
    for (const auto& club : career.allTeams) {
        const int played = club.wins + club.draws + club.losses;
        if (played < 4) continue;
        int score = club.points * 5 + club.morale - max(0, club.goalsAgainst - club.goalsFor) * 2;
        if (club.morale <= 42) pressureClubs++;
        if (score < worstScore) {
            worstScore = score;
            mostPressured = &club;
        }
    }
    summary.pressureClubs = pressureClubs;
    if (mostPressured && career.currentWeek % 3 == 0) {
        career.addNews("[Mundo] " + mostPressured->name + " entra en zona de presion institucional.");
        pushHeadline(summary, "Mundo: " + mostPressured->name + " queda bajo fuerte presion por resultados.");
    }

    if (career.currentWeek % 4 == 0) {
        const Team* clubOwner = nullptr;
        const Player* prospect = nullptr;
        int upside = -1;
        for (const auto& club : career.allTeams) {
            for (const auto& player : club.players) {
                if (player.age > 21) continue;
                const int score = player.potential - player.skill + player.currentForm / 12;
                if (score > upside) {
                    upside = score;
                    prospect = &player;
                    clubOwner = &club;
                }
            }
        }
        if (prospect && clubOwner) {
            career.addNews("[Mundo] El scouting destaca a " + prospect->name + " de " + clubOwner->name + ".");
            pushHeadline(summary, "Talento emergente: " + prospect->name + " gana atencion en el mercado.");
        }
    }

    return summary;
}

int updateSeasonRecords(Career& career, const LeagueTable& table) {
    int updates = 0;
    if (table.teams.empty()) return updates;
    const string division = divisionDisplay(table.ruleId);

    Team* pointsLeader = table.teams.front();
    if (pointsLeader &&
        updateRecord(career.historicalRecords,
                     division + " - Mas puntos",
                     pointsLeader->name,
                     pointsLeader->name,
                     career.currentSeason,
                     pointsLeader->points,
                     "Puntos en una temporada regular",
                     false)) {
        career.addNews("[Record] " + pointsLeader->name + " fija una nueva marca de puntos en " + division + ".");
        updates++;
    }

    Team* attackLeader = *max_element(table.teams.begin(), table.teams.end(), [](const Team* left, const Team* right) {
        return left->goalsFor < right->goalsFor;
    });
    if (attackLeader &&
        updateRecord(career.historicalRecords,
                     division + " - Mas goles",
                     attackLeader->name,
                     attackLeader->name,
                     career.currentSeason,
                     attackLeader->goalsFor,
                     "Goles convertidos por un club",
                     false)) {
        career.addNews("[Record] " + attackLeader->name + " rompe el techo goleador de " + division + ".");
        updates++;
    }

    Team* defenseLeader = *min_element(table.teams.begin(), table.teams.end(), [](const Team* left, const Team* right) {
        return left->goalsAgainst < right->goalsAgainst;
    });
    if (defenseLeader &&
        updateRecord(career.historicalRecords,
                     division + " - Mejor defensa",
                     defenseLeader->name,
                     defenseLeader->name,
                     career.currentSeason,
                     defenseLeader->goalsAgainst,
                     "Menos goles concedidos por un club",
                     true)) {
        career.addNews("[Record] " + defenseLeader->name + " registra la mejor defensa historica de " + division + ".");
        updates++;
    }

    const Team* scorerTeam = nullptr;
    const Player* scorer = nullptr;
    for (const Team* club : table.teams) {
        if (!club) continue;
        for (const auto& player : club->players) {
            if (!scorer || player.goals > scorer->goals) {
                scorer = &player;
                scorerTeam = club;
            }
        }
    }
    if (scorer && scorerTeam &&
        updateRecord(career.historicalRecords,
                     division + " - Goleador",
                     scorer->name,
                     scorerTeam->name,
                     career.currentSeason,
                     scorer->goals,
                     "Goles de un jugador en temporada",
                     false)) {
        career.addNews("[Record] " + scorer->name + " firma un nuevo techo goleador en " + division + ".");
        updates++;
    }

    if (career.historicalRecords.size() > 40) {
        career.historicalRecords.erase(career.historicalRecords.begin(),
                                       career.historicalRecords.begin() +
                                           static_cast<long long>(career.historicalRecords.size() - 40));
    }
    return updates;
}

string formatPromiseSummary(const Career& career, size_t maxLines) {
    if (career.activePromises.empty()) return "Sin promesas activas.";
    ostringstream out;
    const size_t limit = min(maxLines, career.activePromises.size());
    for (size_t i = 0; i < limit; ++i) {
        if (i) out << "\r\n";
        out << "- " << promiseLabel(career.activePromises[i]);
    }
    return out.str();
}

string formatHistoricalRecordSummary(const Career& career, size_t maxLines) {
    if (career.historicalRecords.empty()) return "Sin records historicos.";
    ostringstream out;
    const size_t total = career.historicalRecords.size();
    const size_t start = total > maxLines ? total - maxLines : 0;
    for (size_t i = start; i < total; ++i) {
        const HistoricalRecord& record = career.historicalRecords[i];
        if (i > start) out << "\r\n";
        out << "- " << record.category << ": " << record.holderName
            << " (" << record.teamName << ", " << record.value << ", T" << record.season << ")";
    }
    return out.str();
}

}  // namespace world_state_service
