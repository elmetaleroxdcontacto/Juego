#include "career/world_state_service.h"

#include "career/career_reports.h"
#include "competition/competition.h"
#include "utils/utils.h"

#include <algorithm>
#include <map>
#include <sstream>

using namespace std;

namespace {

const char* kWorldRulesPath = "data/configs/world_rules.csv";
const char* kScoutingRegionsPath = "data/configs/scouting_regions.csv";

struct WorldConfigCache {
    bool loaded = false;
    map<string, int> worldRules;
    vector<string> scoutingRegions;
};

WorldConfigCache& configCache() {
    static WorldConfigCache cache;
    return cache;
}

void loadConfiguredWorldData() {
    WorldConfigCache& cache = configCache();
    if (cache.loaded) return;
    cache.loaded = true;
    cache.worldRules = {
        {"background_leader_story_chance", 14},
        {"background_pressure_story_chance", 10},
        {"background_manager_review_chance", 8},
        {"background_youth_promotion_chance", 12},
        {"background_injury_story_chance", 8},
        {"weekly_pressure_headline_mod", 3},
        {"manager_change_chance", 7},
        {"world_talent_headline_mod", 4},
        {"world_market_headline_mod", 5},
        {"scouting_network_bonus", 8},
    };
    cache.scoutingRegions = {"Metropolitana", "Centro", "Sur", "Norte", "Patagonia", "Internacional"};

    vector<string> lines;
    if (readTextFileLines(kWorldRulesPath, lines) && lines.size() > 1) {
        for (size_t i = 1; i < lines.size(); ++i) {
            const string line = trim(lines[i]);
            if (line.empty()) continue;
            const vector<string> cols = splitCsvLine(line);
            if (cols.size() < 2) continue;
            const string key = toLower(trim(cols[0]));
            if (key.empty()) continue;
            cache.worldRules[key] = parseAge(cols[1]);
        }
    }

    lines.clear();
    if (readTextFileLines(kScoutingRegionsPath, lines) && lines.size() > 1) {
        cache.scoutingRegions.clear();
        for (size_t i = 1; i < lines.size(); ++i) {
            const string line = trim(lines[i]);
            if (line.empty()) continue;
            const vector<string> cols = splitCsvLine(line);
            if (cols.empty()) continue;
            const string name = trim(cols[0]);
            if (!name.empty() && find(cache.scoutingRegions.begin(), cache.scoutingRegions.end(), name) == cache.scoutingRegions.end()) {
                cache.scoutingRegions.push_back(name);
            }
        }
        if (cache.scoutingRegions.empty()) {
            cache.scoutingRegions = {"Metropolitana", "Centro", "Sur", "Norte", "Patagonia", "Internacional"};
        }
    }
}

int configuredWorldRule(const string& key, int defaultValue) {
    loadConfiguredWorldData();
    const auto& rules = configCache().worldRules;
    auto it = rules.find(toLower(trim(key)));
    return it == rules.end() ? defaultValue : it->second;
}

vector<string> configuredScoutingRegions() {
    loadConfiguredWorldData();
    return configCache().scoutingRegions;
}

string replacementCoachName(const Team& team, int salt) {
    static const vector<string> firstNames = {"Nicolas", "Mauricio", "Sebastian", "Cristian", "Patricio", "Rodolfo"};
    static const vector<string> lastNames = {"Araya", "Cisternas", "Yanez", "Henriquez", "Orellana", "Fuenzalida"};
    int hash = salt;
    for (char ch : normalizeTeamId(team.name)) hash += static_cast<unsigned char>(ch);
    return firstNames[static_cast<size_t>(hash % static_cast<int>(firstNames.size()))] + " " +
           lastNames[static_cast<size_t>((hash / 5) % static_cast<int>(lastNames.size()))];
}

string replacementCoachStyle(const Team& team) {
    if (team.matchInstruction == "Juego directo") return "Vertical";
    if (team.tactics == "Pressing") return "Presion";
    if (team.tactics == "Defensive") return "Contencion";
    return "Control";
}

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

int seasonDeadlineWeek(const Career& career) {
    return max(career.currentWeek, static_cast<int>(career.schedule.size()));
}

void pushHeadline(WorldPulseSummary& summary, const string& line) {
    if (line.empty()) return;
    if (summary.headlines.size() < 8) summary.headlines.push_back(line);
}

void pushSeasonSummary(vector<string>* summaryLines, const string& line) {
    if (!summaryLines || line.empty()) return;
    summaryLines->push_back(line);
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

    const int seasonDeadline = seasonDeadlineWeek(career);
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
    if (mostPressured && career.currentWeek % max(1, configuredWorldRule("weekly_pressure_headline_mod", 3)) == 0) {
        career.addNews("[Mundo] " + mostPressured->name + " entra en zona de presion institucional.");
        career.addInboxItem(mostPressured->name + " queda en la mira por su mala racha.", "Mundo");
        pushHeadline(summary, "Mundo: " + mostPressured->name + " queda bajo fuerte presion por resultados.");
    }

    if (career.currentWeek % max(1, configuredWorldRule("world_talent_headline_mod", 4)) == 0) {
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

    if (pressureClubs >= 3 && career.currentWeek % max(1, configuredWorldRule("world_talent_headline_mod", 4)) == 2) {
        career.addNews("[Mundo] Varias directivas entran en modo revision por un arranque irregular.");
        pushHeadline(summary, "Clima institucional: varias bancas quedan bajo observacion esta semana.");
    }

    if (mostPressured && career.currentWeek % max(1, configuredWorldRule("world_market_headline_mod", 5)) == 0) {
        career.addNews("[Mundo] Rumor de banquillo: " + mostPressured->name + " estudia cambios tras semanas de presion.");
        pushHeadline(summary, "Banquillos: " + mostPressured->name + " aparece en el radar de rumores.");
    }

    if (career.currentWeek % max(1, configuredWorldRule("world_market_headline_mod", 5)) == 0) {
        const Team* seller = nullptr;
        const Player* target = nullptr;
        int marketScore = -1;
        for (const auto& club : career.allTeams) {
            for (const auto& player : club.players) {
                if (player.age > 29 || player.injured) continue;
                int score = player.skill + max(0, player.potential - player.skill) +
                            (player.wantsToLeave ? 12 : 0) + max(0, 52 - player.happiness);
                if (score > marketScore) {
                    marketScore = score;
                    seller = &club;
                    target = &player;
                }
            }
        }
        if (seller && target) {
            career.addNews("[Mundo] Mercado: crece el interes por " + target->name + " de " + seller->name + ".");
            career.addInboxItem("El mercado se mueve alrededor de " + target->name + " (" + seller->name + ").", "Mercado");
            pushHeadline(summary, "Mercado: " + target->name + " pasa a ser uno de los nombres de la semana.");
        }
    }

    for (auto& club : career.allTeams) {
        ensureTeamIdentity(club);
        club.headCoachTenureWeeks = clampInt(club.headCoachTenureWeeks + 1, 0, 520);
        if (club.scoutingChief <= 58 && club.budget > 90000 && randInt(1, 100) <= configuredWorldRule("background_staff_hire_chance", 10)) {
            club.scoutingChief = clampInt(club.scoutingChief + randInt(2, 5), 1, 99);
            club.scoutingChiefName.clear();
            ensureTeamIdentity(club);
            if (&club == career.myTeam) career.addInboxItem("La directiva autoriza reforzar scouting con " + club.scoutingChiefName + ".", "Staff");
        }
        if (club.performanceAnalyst <= 57 && club.budget > 70000 && randInt(1, 100) <= configuredWorldRule("background_staff_hire_chance", 10)) {
            club.performanceAnalyst = clampInt(club.performanceAnalyst + randInt(2, 5), 1, 99);
            club.performanceAnalystName.clear();
            ensureTeamIdentity(club);
            if (&club == career.myTeam) career.addInboxItem("El club suma al analista " + club.performanceAnalystName + ".", "Staff");
        }
        const int played = club.wins + club.draws + club.losses;
        if (played < 4) continue;
        int pressure = 55 - club.morale + max(0, club.goalsAgainst - club.goalsFor) * 2;
        pressure += max(0, teamPrestigeScore(club) - club.points * 2 / max(1, played));
        if (&club == career.myTeam) pressure += max(0, 50 - career.boardConfidence) / 3;
        club.jobSecurity = clampInt(72 - pressure / 2 + club.headCoachReputation / 10, 8, 92);
        club.headCoachStyle = replacementCoachStyle(club);

        if (club.jobSecurity <= 30 && randInt(1, 100) <= configuredWorldRule("background_manager_review_chance", 8)) {
            career.addNews("[Mundo] " + club.name + " entra en revision tecnica con " + club.headCoachName + ".");
            pushHeadline(summary, "Banquillo caliente: " + club.name + " evalua el trabajo de " + club.headCoachName + ".");
        }
        if (club.jobSecurity <= 18 && randInt(1, 100) <= configuredWorldRule("manager_change_chance", 7)) {
            const string oldCoach = club.headCoachName;
            club.headCoachName = replacementCoachName(club, career.currentWeek + club.points);
            club.headCoachReputation = clampInt(club.headCoachReputation + randInt(-4, 7), 25, 95);
            club.headCoachTenureWeeks = 0;
            club.jobSecurity = 64;
            club.transferPolicy = club.debt > club.sponsorWeekly * 18 ? "Reconstruccion austera" : "Rearme competitivo";
            career.addNews("[Mundo] " + club.name + " cambia de entrenador: sale " + oldCoach + " y llega " + club.headCoachName + ".");
            career.addInboxItem(club.name + " cambia de entrenador. Nuevo estilo: " + club.headCoachStyle + ".", "Mundo");
            pushHeadline(summary, "Cambio de banquillo: " + club.name + " presenta a " + club.headCoachName + ".");
        }
    }

    return summary;
}

int resolveSeasonCarryover(Career& career, vector<string>* summaryLines) {
    if (!career.myTeam || career.activePromises.empty()) return 0;

    Team& team = *career.myTeam;
    const int seasonEnd = seasonDeadlineWeek(career);
    int resolved = 0;
    for (auto& promise : career.activePromises) {
        if (promise.fulfilled || promise.failed) continue;
        promise.deadlineWeek = min(promise.deadlineWeek, seasonEnd);

        if (promise.category == "Minutos") {
            Player* player = findPlayerByName(team, promise.subjectName);
            const int targetStarts = targetStartsForPromise(promise.target);
            promise.progress = player ? player->startsThisSeason : 0;
            if (player && promise.progress >= targetStarts) {
                promise.fulfilled = true;
                player->happiness = clampInt(player->happiness + 4, 1, 99);
                player->moraleMomentum = clampInt(player->moraleMomentum + 2, -25, 25);
                team.morale = clampInt(team.morale + 1, 0, 100);
                career.addNews("Cierre de temporada: " + player->name + " termina el curso con su promesa de minutos cumplida.");
                pushSeasonSummary(summaryLines, "Promesa cerrada: " + player->name + " recibe los minutos comprometidos.");
            } else {
                promise.failed = true;
                if (player) {
                    player->happiness = clampInt(player->happiness - 7, 1, 99);
                    player->moraleMomentum = clampInt(player->moraleMomentum - 3, -25, 25);
                    player->wantsToLeave = player->ambition >= 55 || player->promisedRole != "Sin promesa";
                }
                team.morale = clampInt(team.morale - 2, 0, 100);
                career.addNews("Cierre de temporada: " + promise.subjectName + " cierra el ano molesto por una promesa de minutos incumplida.");
                pushSeasonSummary(summaryLines, "Promesa rota: " + promise.subjectName + " termina la temporada sin los minutos acordados.");
            }
            resolved++;
            continue;
        }

        if (promise.category == "Fichaje") {
            promise.progress = qualityPlayersAtPosition(team, promise.target);
            if (promise.progress >= 3) {
                promise.fulfilled = true;
                career.boardConfidence = clampInt(career.boardConfidence + 2, 0, 100);
                career.addNews("Cierre de temporada: el club termina con profundidad suficiente en " + promise.target + ".");
                pushSeasonSummary(summaryLines, "Promesa cerrada: la plantilla ya tiene relevo suficiente en " + promise.target + ".");
            } else {
                promise.failed = true;
                career.boardConfidence = clampInt(career.boardConfidence - 3, 0, 100);
                team.morale = clampInt(team.morale - 1, 0, 100);
                career.addNews("Cierre de temporada: sigue sin llegar el refuerzo prometido para " + promise.target + ".");
                pushSeasonSummary(summaryLines, "Promesa rota: la necesidad en " + promise.target + " queda abierta al final del curso.");
            }
            resolved++;
            continue;
        }

        if (promise.category == "Competicion") {
            int targetRank = 1;
            try {
                targetRank = max(1, stoi(promise.target));
            } catch (...) {
                targetRank = 1;
            }
            const int currentRank = max(1, career.currentCompetitiveRank());
            promise.progress = max(0, targetRank - currentRank + 1);
            if (currentRank <= targetRank) {
                promise.fulfilled = true;
                career.managerReputation = clampInt(career.managerReputation + 3, 1, 100);
                team.morale = clampInt(team.morale + 2, 0, 100);
                career.addNews("Cierre de temporada: " + team.name + " termina dentro del objetivo competitivo fijado.");
                pushSeasonSummary(summaryLines, "Promesa cerrada: " + team.name + " cumple la meta de tabla del curso.");
            } else {
                promise.failed = true;
                career.managerReputation = clampInt(career.managerReputation - 2, 1, 100);
                career.boardConfidence = clampInt(career.boardConfidence - 4, 0, 100);
                team.morale = clampInt(team.morale - 3, 0, 100);
                career.addNews("Cierre de temporada: " + team.name + " termina el curso por debajo del objetivo comprometido.");
                pushSeasonSummary(summaryLines, "Promesa rota: " + team.name + " no alcanza la meta de tabla al final del ano.");
            }
            resolved++;
        }
    }

    career.activePromises.clear();
    return resolved;
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

int worldRuleValue(const string& key, int defaultValue) {
    return configuredWorldRule(key, defaultValue);
}

vector<string> listConfiguredScoutingRegions() {
    return configuredScoutingRegions();
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
