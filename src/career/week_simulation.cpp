#include "career/week_simulation.h"

#include "ai/team_ai.h"
#include "career/match_analysis_store.h"
#include "career/dressing_room_service.h"
#include "career/career_reports.h"
#include "career/career_runtime.h"
#include "career/season_transition.h"
#include "career/career_support.h"
#include "career/player_development.h"
#include "career/team_management.h"
#include "career/world_state_service.h"
#include "competition.h"
#include "simulation.h"
#include "transfers/negotiation_system.h"
#include "transfers/transfer_market.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <unordered_map>

using namespace std;

namespace {

int teamRank(const LeagueTable& table, const Team* team) {
    for (size_t i = 0; i < table.teams.size(); ++i) {
        if (table.teams[i] == team) return static_cast<int>(i) + 1;
    }
    return -1;
}

bool isKeyMatch(const LeagueTable& table, const Team* home, const Team* away) {
    int homeRank = teamRank(table, home);
    int awayRank = teamRank(table, away);
    if (homeRank <= 0 || awayRank <= 0) return false;
    if (homeRank <= 3 || awayRank <= 3) return true;
    return abs(homeRank - awayRank) <= 2;
}

long long divisionBaseIncome(const string& division) {
    return getCompetitionConfig(division).baseIncome;
}

int divisionWageFactor(const string& division) {
    return getCompetitionConfig(division).wageFactor;
}

long long weeklyWage(const Team& team) {
    long long total = 0;
    for (const auto& player : team.players) total += player.wage;
    return total * divisionWageFactor(team.division) / 100;
}

void applyWeeklyFinances(Career& career, const vector<int>& pointsBefore) {
    unordered_map<Team*, int> homeGames;
    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        for (const auto& match : career.schedule[static_cast<size_t>(career.currentWeek - 1)]) {
            homeGames[career.activeTeams[static_cast<size_t>(match.first)]]++;
        }
    }

    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        int pointsDelta = team->points - pointsBefore[i];
        if (pointsDelta >= 3) {
            team->fanBase = clampInt(team->fanBase + 1, 10, 99);
        } else if (pointsDelta == 0 && team->fanBase > 12 && randInt(1, 100) <= 30) {
            team->fanBase--;
        }

        long long ticketIncome =
            static_cast<long long>(homeGames[team]) * (team->fanBase * 2500LL + team->stadiumLevel * 7000LL);
        long long seasonTickets = (career.currentWeek % 4 == 1) ? team->fanBase * 900LL : 0LL;
        long long merchandising = static_cast<long long>(team->fanBase) * 350LL +
                                  static_cast<long long>(teamPrestigeScore(*team)) * 180LL +
                                  static_cast<long long>(max(0, team->goalsFor - team->goalsAgainst)) * 120LL;
        long long sponsorActivation = (pointsDelta >= 3 ? 3500LL : 0LL) + (team->fanBase >= 60 ? 2000LL : 0LL);
        long long sponsor = team->sponsorWeekly + max(0, pointsDelta) * 800LL + sponsorActivation;
        long long performanceBonus = pointsDelta * 4000LL;
        long long solidarity = randInt(0, 3000);
        long long income = divisionBaseIncome(team->division) + sponsor + ticketIncome + seasonTickets +
                           merchandising + performanceBonus + solidarity;
        long long wages = weeklyWage(*team);
        long long debtPayment = min(team->debt, max(0LL, income / 8));
        team->debt -= debtPayment;
        long long debtInterest = max(0LL, team->debt / 250);
        long long infrastructure =
            (team->trainingFacilityLevel + team->youthFacilityLevel + team->stadiumLevel - 3) * 1500LL;
        long long net = income - wages - debtPayment - debtInterest - infrastructure;
        team->budget = max(0LL, team->budget + net);

        if (career.currentWeek % 8 == 0 && pointsDelta >= 3) {
            team->sponsorWeekly += max(500LL, team->fanBase * 30LL);
        }

        if (career.myTeam == team) {
            ostringstream out;
            out << "Finanzas semanales: +" << income << " (entradas " << ticketIncome << ", abonos "
                << seasonTickets << ", merch " << merchandising << ", sponsor " << sponsor << ")"
                << " / -" << wages << " salarios"
                << " / -" << debtPayment << " deuda"
                << " / -" << debtInterest << " interes"
                << " / -" << infrastructure << " infraestructura"
                << " = " << net;
            emitUiMessage(out.str());
        }
    }
}

void generateManagerCareerEvents(Career& career) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    int field = max(1, career.currentCompetitiveFieldSize());
    if (rank > 0 && rank <= max(2, field / 4) && randInt(1, 100) <= 18) {
        career.addNews("Entrevista: la prensa describe a " + career.managerName + " como un DT " +
                       managerStyleLabel(*career.myTeam) + ".");
    }
    int youthContributors = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.age <= 21 && player.matchesPlayed >= 4) youthContributors++;
    }
    if (youthContributors >= 2 && randInt(1, 100) <= 16) {
        career.addNews("Perfil de manager: la prensa valora la apuesta juvenil de " + career.managerName + ".");
    }
    if (career.managerReputation >= 58 && rank > 0 && rank <= career.boardExpectedFinish &&
        randInt(1, 100) <= 12) {
        vector<Team*> jobs = buildJobMarket(career, false);
        if (!jobs.empty()) {
            career.addNews("Rumor de banquillo: " + jobs.front()->name + " sigue a " + career.managerName + ".");
        }
    }
}

void weeklyDashboard(const Career& career) {
    if (!career.myTeam) return;
    LeagueTable table = buildRelevantCompetitionTable(career);
    int rank = teamRank(table, career.myTeam);
    int injured = 0;
    int suspended = 0;
    int expiring = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.injured) injured++;
        if (player.matchesSuspended > 0) suspended++;
        if (player.contractWeeks > 0 && player.contractWeeks <= 4) expiring++;
    }

    emitUiMessage("");
    emitUiMessage("--- Resumen Semanal ---");
    {
        ostringstream out;
        out << "Posicion: " << rank << " | Pts: " << career.myTeam->points
            << " | Moral: " << career.myTeam->morale
            << " | Presupuesto: $" << career.myTeam->budget
            << " | Directiva: " << career.boardConfidence << "/100";
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Sponsor: $" << career.myTeam->sponsorWeekly << " | Deuda: $" << career.myTeam->debt
            << " | Infraestructura E/C/T: " << career.myTeam->stadiumLevel << "/"
            << career.myTeam->youthFacilityLevel << "/" << career.myTeam->trainingFacilityLevel;
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Lesionados: " << injured
            << " | Suspendidos: " << suspended
            << " | Contratos por vencer (<=4 sem): " << expiring
            << " | Shortlist: " << career.scoutingShortlist.size()
            << " | Promesas: " << career.activePromises.size();
        emitUiMessage(out.str());
    }
    emitUiMessage("Identidad: " + career.myTeam->clubStyle +
                  " | Cantera: " + career.myTeam->youthIdentity +
                  " | Rival: " +
                  (career.myTeam->primaryRival.empty() ? string("-") : career.myTeam->primaryRival));
    emitUiMessage("Estado por lineas: " + lineMap(*career.myTeam));
    emitUiMessage("Rival proximo: " + buildOpponentReport(career));
    if (!career.boardMonthlyObjective.empty()) {
        emitUiMessage("Objetivo mensual: " + career.boardMonthlyObjective +
                      " | " + to_string(career.boardMonthlyProgress) + "/" +
                      to_string(career.boardMonthlyTarget));
    }
    if (!career.cupChampion.empty()) {
        emitUiMessage("Copa: campeon " + career.cupChampion);
    } else if (career.cupActive) {
        emitUiMessage("Copa: ronda " + to_string(career.cupRound + 1) +
                      " | vivos " + to_string(career.cupRemainingTeams.size()));
    }
}

void applyClubEvent(Career& career) {
    if (!career.myTeam || randInt(1, 100) > 15) return;
    int event = randInt(1, 6);
    if (event == 1) {
        long long bonus = 50000 + randInt(0, 30000);
        career.myTeam->budget += bonus;
        career.addNews("Nuevo patrocinio para " + career.myTeam->name + " por $" + to_string(bonus) + ".");
        emitUiMessage("[Evento] Patrocinio sorpresa: +" + to_string(bonus));
        return;
    }
    if (event == 2) {
        career.myTeam->morale = clampInt(career.myTeam->morale - 5, 0, 100);
        career.addNews("La hinchada presiona a " + career.myTeam->name + " tras los ultimos resultados.");
        emitUiMessage("[Evento] Protesta de hinchas: moral -5.");
        return;
    }
    if (event == 3) {
        if (career.myTeam->players.empty()) return;
        int index = randInt(0, static_cast<int>(career.myTeam->players.size()) - 1);
        Player& player = career.myTeam->players[static_cast<size_t>(index)];
        player.injured = true;
        player.injuryType = "Leve";
        player.injuryWeeks = randInt(1, 2);
        player.injuryHistory++;
        career.addNews(player.name + " sufre una lesion leve en entrenamiento.");
        emitUiMessage("[Evento] Accidente en entrenamiento: " + player.name +
                      " fuera " + to_string(player.injuryWeeks) + " semanas.");
        return;
    }
    if (event == 4) {
        for (auto& player : career.myTeam->players) {
            if (player.age > 29 || player.startsThisSeason > 1) continue;
            player.happiness = clampInt(player.happiness - 5, 1, 99);
            player.unhappinessWeeks = clampInt(player.unhappinessWeeks + 1, 0, 52);
            player.socialGroup = "Frustrados";
            career.addNews("Vestuario: " + player.name + " pide una conversacion por falta de minutos.");
            emitUiMessage("[Evento] Vestuario tenso: " + player.name + " reclama mas protagonismo.");
            return;
        }
    }
    if (event == 5) {
        const Player* best = nullptr;
        for (const auto& player : career.myTeam->players) {
            if (player.injured) continue;
            if (!best || player.skill > best->skill) best = &player;
        }
        if (best) {
            int idx = static_cast<int>(best - &career.myTeam->players[0]);
            Player& player = career.myTeam->players[static_cast<size_t>(idx)];
            player.wantsToLeave = player.ambition >= 60;
            player.happiness = clampInt(player.happiness - 3, 1, 99);
            career.addNews("Mercado: un club grande empieza a seguir a " + player.name + ".");
            emitUiMessage("[Evento] Mercado: aumenta el interes externo por " + player.name + ".");
            return;
        }
    }

    int maxSquad = getCompetitionConfig(career.myTeam->division).maxSquadSize;
    if (maxSquad > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquad) return;
    int minSkill = 0;
    int maxSkill = 0;
    getDivisionSkillRange(career.myTeam->division, minSkill, maxSkill);
    int youthBoost = max(0, career.myTeam->youthFacilityLevel - 1);
    Player youth = makeRandomPlayer("MED", minSkill + youthBoost, maxSkill + youthBoost, 16, 18);
    youth.potential = clampInt(youth.skill + randInt(8 + youthBoost, 15 + youthBoost), youth.skill, 99);
    career.myTeam->addPlayer(youth);
    career.addNews("La cantera promociona a " + youth.name + " en " + career.myTeam->name + ".");
    emitUiMessage("[Evento] Cantera: se unio " + youth.name + " (pot " + to_string(youth.potential) + ").");
}

struct TeamTableSnapshot {
    int points;
    int goalsFor;
    int goalsAgainst;
    int awayGoals;
    int wins;
    int draws;
    int losses;
    int yellowCards;
    int redCards;
    vector<HeadToHeadRecord> headToHead;
};

TeamTableSnapshot captureTableState(const Team& team) {
    return {team.points, team.goalsFor, team.goalsAgainst, team.awayGoals, team.wins, team.draws,
            team.losses, team.yellowCards, team.redCards, team.headToHead};
}

void restoreTableState(Team& team, const TeamTableSnapshot& snapshot) {
    team.points = snapshot.points;
    team.goalsFor = snapshot.goalsFor;
    team.goalsAgainst = snapshot.goalsAgainst;
    team.awayGoals = snapshot.awayGoals;
    team.wins = snapshot.wins;
    team.draws = snapshot.draws;
    team.losses = snapshot.losses;
    team.yellowCards = snapshot.yellowCards;
    team.redCards = snapshot.redCards;
    team.headToHead = snapshot.headToHead;
}

void storeMatchAnalysis(Career& career,
                        const Team& home,
                        const Team& away,
                        const MatchResult& result,
                        bool cupMatch) {
    career_match_analysis::storeMatchAnalysis(career, home, away, result, cupMatch);
}

void simulateSeasonCupRound(Career& career) {
    if (!career.cupActive) return;
    vector<Team*> alive;
    for (const auto& name : career.cupRemainingTeams) {
        Team* team = career.findTeamByName(name);
        if (team && team->division == career.activeDivision) alive.push_back(team);
    }
    if (alive.size() <= 1) {
        career.cupActive = false;
        if (!alive.empty()) {
            career.cupChampion = alive.front()->name;
            career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        }
        return;
    }

    career.cupRound++;
    emitUiMessage("");
    emitUiMessage("--- Copa de temporada: ronda " + to_string(career.cupRound) + " ---");
    vector<string> nextRound;
    if (alive.size() % 2 == 1) {
        Team* bye = alive.back();
        nextRound.push_back(bye->name);
        alive.pop_back();
        emitUiMessage("Pase libre: " + bye->name);
    }

    for (size_t i = 0; i < alive.size(); i += 2) {
        Team* home = alive[i];
        Team* away = alive[i + 1];
        TeamTableSnapshot homeSnap = captureTableState(*home);
        TeamTableSnapshot awaySnap = captureTableState(*away);
        bool verbose = (home == career.myTeam || away == career.myTeam);
        emitUiMessage(home->name + " vs " + away->name);
        MatchResult result = playMatch(*home, *away, verbose, true, true);
        restoreTableState(*home, homeSnap);
        restoreTableState(*away, awaySnap);
        storeMatchAnalysis(career, *home, *away, result, true);

        Team* winner = home;
        if (result.awayGoals > result.homeGoals) {
            winner = away;
        } else if (result.homeGoals == result.awayGoals) {
            winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
            emitUiMessage("Gana por penales: " + winner->name);
        }
        nextRound.push_back(winner->name);
    }

    career.cupRemainingTeams = nextRound;
    if (career.cupRemainingTeams.size() == 1) {
        career.cupActive = false;
        career.cupChampion = career.cupRemainingTeams.front();
        career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        emitUiMessage("Campeon de la copa: " + career.cupChampion);
    }
}

void updateSquadDynamics(Career& career, int pointsDelta) {
    DressingRoomSnapshot snapshot = dressing_room_service::applyWeeklyUpdate(career, pointsDelta);
    if (!snapshot.summary.empty()) {
        emitUiMessage("[Vestuario] " + snapshot.summary);
    }
    for (size_t i = 0; i < snapshot.alerts.size() && i < 2; ++i) {
        emitUiMessage("[Vestuario] " + snapshot.alerts[i]);
    }
}

void runMonthlyDevelopment(Career& career) {
    if (career.currentWeek <= 0 || career.currentWeek % 4 != 0) return;
    for (auto* team : career.activeTeams) {
        int improved = 0;
        int eliteImproved = 0;
        for (auto& player : team->players) {
            if (player.age > 21 || player.skill >= player.potential) continue;
            int chance = 6 + max(0, team->youthCoach - 55) / 10 + max(0, team->trainingFacilityLevel - 1) * 2;
            chance += max(0, player.professionalism - 55) / 15;
            if (player.developmentPlan == "Finalizacion" || player.developmentPlan == "Creatividad" ||
                player.developmentPlan == "Defensa" || player.developmentPlan == "Reflejos") {
                chance += 2;
            }
            if (player.promisedRole == "Proyecto") chance += 2;
            if (playerHasTrait(player, "Proyecto") || playerHasTrait(player, "Competidor")) chance += 2;
            if (randInt(1, 100) > clampInt(chance, 4, 28)) continue;

            int gain = 1;
            if (player.age <= 19 && player.potential - player.skill >= 10 && randInt(1, 100) <= 28) gain++;
            player.skill = min(100, player.skill + gain);
            string normalizedPosition = normalizePosition(player.position);
            if (normalizedPosition == "ARQ" || normalizedPosition == "DEF") {
                player.defense = min(100, player.defense + 1);
            } else if (normalizedPosition == "MED") {
                player.attack = min(100, player.attack + 1);
                player.defense = min(100, player.defense + 1);
            } else {
                player.attack = min(100, player.attack + 1);
            }
            if (player.developmentPlan == "Liderazgo") {
                player.leadership = min(99, player.leadership + 1);
                player.professionalism = min(99, player.professionalism + 1);
            } else if (player.developmentPlan == "Creatividad") {
                player.setPieceSkill = min(99, player.setPieceSkill + 1);
            } else if (player.developmentPlan == "Fisico") {
                player.stamina = min(100, player.stamina + 1);
                player.fitness = min(player.stamina, player.fitness + 1);
            }
            improved++;
            if (gain > 1) eliteImproved++;
        }

        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if ((maxSquad <= 0 || static_cast<int>(team->players.size()) < maxSquad) &&
            randInt(1, 100) <= 4 + team->youthFacilityLevel * 3) {
            player_dev::addYouthPlayers(*team, 1);
            if (team == career.myTeam) {
                career.addNews("La cantera suma un nuevo prospecto desde la region " + team->youthRegion + ".");
            }
        }
        if (team == career.myTeam && improved > 0) {
            career.addNews("Informe juvenil: " + to_string(improved) + " jugador(es) joven(es) mejoran este mes.");
            if (eliteImproved > 0) {
                career.addNews("Informe de cantera: " + to_string(eliteImproved) +
                               " prospecto(s) muestran una aceleracion especial.");
            }
        }
    }
}

void updateShortlistAlerts(Career& career) {
    if (!career.myTeam || career.scoutingShortlist.empty() || career.currentWeek % 4 != 0) return;

    vector<string> active;
    for (const auto& item : career.scoutingShortlist) {
        auto parts = splitByDelimiter(item, '|');
        if (parts.size() < 2) continue;
        Team* seller = career.findTeamByName(parts[0]);
        if (!seller) continue;
        int index = team_mgmt::playerIndexByName(*seller, parts[1]);
        if (index < 0) continue;
        const Player& player = seller->players[static_cast<size_t>(index)];
        active.push_back(item);
        if (player.contractWeeks <= 12) {
            career.addNews("Alerta de shortlist: " + player.name + " entra en ventana de precontrato con " +
                           seller->name + ".");
        } else if (player.value <= player.releaseClause * 60 / 100) {
            career.addNews("Alerta de shortlist: " + player.name + " mantiene un costo accesible en " +
                           seller->name + ".");
        }
    }
    career.scoutingShortlist = active;
}

const Player* leadingForward(const Team& team) {
    const Player* best = nullptr;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != "DEL") continue;
        if (!best || player.skill > best->skill) best = &player;
    }
    return best;
}

void addSquadAlerts(Career& career) {
    if (!career.myTeam) return;
    int defenseFitness = averageFitnessForLine(*career.myTeam, "DEF");
    int midfieldFitness = averageFitnessForLine(*career.myTeam, "MED");
    int attackFitness = averageFitnessForLine(*career.myTeam, "DEL");
    if (defenseFitness < 58 || midfieldFitness < 58 || attackFitness < 58) {
        string line = (defenseFitness <= midfieldFitness && defenseFitness <= attackFitness)
                          ? "la linea defensiva"
                          : (midfieldFitness <= attackFitness ? "el mediocampo" : "el frente de ataque");
        career.addNews("Alerta fisica: " + line + " llega exigida a la proxima fecha.");
    }
    const Player* forward = leadingForward(*career.myTeam);
    if (forward && forward->matchesPlayed >= 5 && forward->goals == 0) {
        career.addNews("Alerta ofensiva: " + forward->name + " ya suma " +
                       to_string(forward->matchesPlayed) + " partido(s) sin marcar.");
    }
    int promiseWarnings = 0;
    for (const auto& player : career.myTeam->players) {
        if (promiseAtRisk(player, career.currentWeek)) promiseWarnings++;
    }
    if (promiseWarnings >= 2) {
        career.addNews("Alerta de vestuario: hay " + to_string(promiseWarnings) +
                       " promesa(s) contractuales bajo revision.");
    }
}

void generateWeeklyNarratives(Career& career, int myTeamPointsDelta) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    int field = max(1, career.currentCompetitiveFieldSize());
    if (rank > 0 && rank <= max(2, field / 4)) {
        career.addNews("La prensa destaca a " + career.myTeam->name + " por su presencia en la zona alta.");
    } else if (rank >= max(2, field - field / 4)) {
        career.addNews("La prensa pone a " + career.myTeam->name + " en la pelea por evitar el fondo.");
    }

    if (myTeamPointsDelta == 3 && career.myTeam->morale >= 65) {
        career.addNews("El vestuario de " + career.myTeam->name + " atraviesa un momento de confianza.");
    } else if (myTeamPointsDelta == 0 && career.boardConfidence <= 30) {
        career.addNews("Crece la tension institucional alrededor de " + career.myTeam->name + ".");
    }

    int promiseAlerts = 0;
    int leaders = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.promisedRole == "Titular" && player.startsThisSeason + 2 < max(2, career.currentWeek * 2 / 3)) {
            promiseAlerts++;
        }
        if (player.promisedRole == "Rotacion" && player.startsThisSeason + 1 < max(1, career.currentWeek / 3)) {
            promiseAlerts++;
        }
        if ((player.leadership >= 72 || playerHasTrait(player, "Lider")) && player.happiness >= 55) {
            leaders++;
        }
    }
    if (promiseAlerts > 0) {
        career.addNews("Se acumulan " + to_string(promiseAlerts) + " promesa(s) de rol bajo presion en el plantel.");
    }
    if (career.myTeam->fanBase >= 65 && myTeamPointsDelta == 3) {
        career.addNews("La aficion responde con entusiasmo y empuja la recaudacion del club.");
    } else if (career.myTeam->fanBase >= 45 && myTeamPointsDelta == 0) {
        career.addNews("La prensa cuestiona la falta de resultados recientes de " + career.myTeam->name + ".");
    }
    if (leaders >= 3 && career.myTeam->morale >= 60) {
        career.addNews("Los lideres del vestuario sostienen un ambiente competitivo en " + career.myTeam->name + ".");
    }
    if (career.myTeam->youthIdentity == "Cantera estructurada") {
        int youthMinutes = 0;
        for (const auto& player : career.myTeam->players) {
            if (player.age <= 20 && player.matchesPlayed > 0) youthMinutes++;
        }
        if (youthMinutes >= 2) {
            career.addNews("La identidad de cantera de " + career.myTeam->name + " gana peso esta semana.");
        }
    }

    const Team* opponent = nextOpponent(career);
    if (opponent) {
        career.addNews("Informe previo: " + buildOpponentReport(career) + ".");
        if (areRivalClubs(*career.myTeam, *opponent)) {
            career.addNews("La semana queda marcada por un clasico ante " + opponent->name + ".");
        }
    }
    if (teamPrestigeScore(*career.myTeam) >= 68 && myTeamPointsDelta == 0) {
        career.addNews("La exigencia institucional aprieta: el entorno de " + career.myTeam->name + " esperaba mas.");
    }

    for (const auto& player : career.myTeam->players) {
        if (player.contractWeeks > 0 && player.contractWeeks <= 4) {
            career.addNews("Contrato al limite: " + player.name + " entra en sus ultimas " +
                           to_string(player.contractWeeks) + " semana(s).");
            break;
        }
    }
    addSquadAlerts(career);
}

void processIncomingOffers(Career& career) {
    if (!career.myTeam || career.myTeam->players.size() <= 18) return;
    bool squadUnrest = false;
    for (const auto& player : career.myTeam->players) {
        if (player.wantsToLeave) {
            squadUnrest = true;
            break;
        }
    }
    if (randInt(1, 100) > (squadUnrest ? 50 : 32)) return;

    vector<int> candidates;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        const Player& player = career.myTeam->players[i];
        if (!player.injured && player.contractWeeks > 8) {
            candidates.push_back(static_cast<int>(i));
            if (player.wantsToLeave) candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) return;

    int index = candidates[static_cast<size_t>(randInt(0, static_cast<int>(candidates.size()) - 1))];
    Player& player = career.myTeam->players[static_cast<size_t>(index)];
    Team* bidder = nullptr;
    int bidderNeed = -100000;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (club.players.size() >= 26) continue;
        if (club.budget < player.value * 9 / 10) continue;
        int need = positionFitScore(player, transfer_market::weakestSquadPosition(club)) +
                   teamPrestigeScore(club) - teamPrestigeScore(*career.myTeam) / 2;
        if (areRivalClubs(club, *career.myTeam)) need += 8;
        if (need > bidderNeed) {
            bidderNeed = need;
            bidder = &club;
        }
    }
    if (!bidder) return;

    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*bidder);
    long long maxOffer = max(player.value, player.value * (105 + randInt(0, 40)) / 100);
    if (areRivalClubs(*bidder, *career.myTeam)) maxOffer = maxOffer * 112 / 100;
    maxOffer = max(maxOffer, static_cast<long long>(player.value * (100 + teamPrestigeScore(*bidder) / 10) / 100));
    long long offer = max(player.value * 9 / 10, maxOffer * 85 / 100);

    emitUiMessage("");
    emitUiMessage("Oferta recibida por " + player.name + " desde " + bidder->name + ": $" + to_string(offer) +
                  " (tope estimado del mercado: $" + to_string(maxOffer) + ")" +
                  (areRivalClubs(*bidder, *career.myTeam) ? " [rival directo]" : ""));

    int choice = (offer >= maxOffer || (player.wantsToLeave && offer >= player.value)) ? 1 : 3;
    long long counter = 0;
    if (incomingOfferDecisionCallback()) {
        IncomingOfferDecision decision = incomingOfferDecisionCallback()(career, player, offer, maxOffer);
        if (decision.action >= 1 && decision.action <= 3) {
            choice = decision.action;
            counter = decision.counterOffer;
        }
    }

    if (choice == 1) {
        career.myTeam->budget += offer;
        bidder->budget = max(0LL, bidder->budget - offer);
        Player moved = player;
        moved.wantsToLeave = false;
        moved.onLoan = false;
        moved.parentClub.clear();
        moved.loanWeeksRemaining = 0;
        bidder->addPlayer(moved);
        emitUiMessage("Transferencia aceptada. " + player.name + " vendido a " + bidder->name + ".");
        career.addNews(player.name + " es vendido a " + bidder->name + " por $" + to_string(offer) + ".");
        team_mgmt::detachPlayerFromSelections(*career.myTeam, player.name);
        team_mgmt::applyDepartureShock(*career.myTeam, player);
        career.myTeam->players.erase(career.myTeam->players.begin() + index);
        return;
    }

    if (choice == 2) {
        if (counter <= maxOffer && bidder->budget >= counter) {
            career.myTeam->budget += counter;
            bidder->budget = max(0LL, bidder->budget - counter);
            Player moved = player;
            moved.wantsToLeave = false;
            moved.onLoan = false;
            moved.parentClub.clear();
            moved.loanWeeksRemaining = 0;
            bidder->addPlayer(moved);
            emitUiMessage("Contraoferta aceptada. " + player.name + " vendido a " + bidder->name + " por $" +
                          to_string(counter));
            career.addNews(player.name + " es vendido a " + bidder->name + " tras contraoferta por $" +
                           to_string(counter) + ".");
            team_mgmt::detachPlayerFromSelections(*career.myTeam, player.name);
            team_mgmt::applyDepartureShock(*career.myTeam, player);
            career.myTeam->players.erase(career.myTeam->players.begin() + index);
        } else {
            emitUiMessage("La contraoferta fue rechazada.");
        }
        return;
    }

    emitUiMessage("Oferta rechazada.");
}

bool shouldAutoRenewContract(const Team& team, const Player& player, long long demandedWage) {
    if (team.budget < demandedWage * 6) return false;
    if (player.wantsToLeave && player.happiness < 45) return false;
    return player.skill >= team.getAverageSkill() - 5 || team.players.size() <= 18;
}

void updateContracts(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        ensureTeamIdentity(*team);
        for (size_t i = 0; i < team->players.size();) {
            Player& player = team->players[i];
            if (player.contractWeeks > 0) player.contractWeeks--;
            if (player.contractWeeks > 0) {
                ++i;
                continue;
            }

            if (team == career.myTeam) {
                long long demandedWage = max(player.wage, wageDemandFor(player));
                if (player.wantsToLeave) demandedWage = demandedWage * 120 / 100;
                if (promiseAtRisk(player, career.currentWeek)) demandedWage = demandedWage * 108 / 100;
                if (player.skill >= team->getAverageSkill()) demandedWage = demandedWage * 110 / 100;
                int demandedWeeks = randInt(78, 182);
                long long demandedClause = max(player.value * 2,
                                               demandedWage * (player.skill >= team->getAverageSkill() ? 48 : 40));
                emitUiMessage("");
                emitUiMessage("Contrato expirado: " + player.name);
                emitUiMessage("Demanda renovar por " + to_string(demandedWeeks) +
                              " semanas | Salario $" + to_string(demandedWage) +
                              " | Clausula $" + to_string(demandedClause));
                if (player.wantsToLeave) {
                    emitUiMessage(player.name + " esta inquieto por su rol en el club y exige mejores condiciones.");
                }
                if (promiseAtRisk(player, career.currentWeek)) {
                    emitUiMessage("Advertencia: " + player.name + " siente que su promesa de rol fue incumplida.");
                }

                bool renew = shouldAutoRenewContract(*team, player, demandedWage);
                if (contractRenewalDecisionCallback()) {
                    renew = contractRenewalDecisionCallback()(career, *team, player, demandedWage, demandedWeeks,
                                                              demandedClause);
                }

                if (renew) {
                    if (team->budget < demandedWage * 6) {
                        emitUiMessage("No hay margen salarial suficiente. " + player.name + " deja el club.");
                        career.addNews(player.name + " deja el club tras no acordar renovacion.");
                        team_mgmt::detachPlayerFromSelections(*team, player.name);
                        team_mgmt::applyDepartureShock(*team, player);
                        team->players.erase(team->players.begin() + static_cast<long long>(i));
                    } else {
                        player.contractWeeks = demandedWeeks;
                        player.wage = demandedWage;
                        player.releaseClause = demandedClause;
                        player.wantsToLeave = false;
                        player.happiness = clampInt(player.happiness + 6, 1, 99);
                        emitUiMessage("Renovado. Nuevo salario $" + to_string(player.wage));
                        career.addNews(player.name + " renueva contrato en " + team->name + ".");
                        ++i;
                    }
                } else {
                    emitUiMessage(player.name + " deja el club.");
                    career.addNews(player.name + " deja el club al finalizar su contrato.");
                    team_mgmt::detachPlayerFromSelections(*team, player.name);
                    team_mgmt::applyDepartureShock(*team, player);
                    team->players.erase(team->players.begin() + static_cast<long long>(i));
                }
            } else {
                if (team->budget > player.wage * 8 &&
                    randInt(1, 100) <= (promiseAtRisk(player, career.currentWeek) ? 45 : 70)) {
                    player.contractWeeks = randInt(52, 156);
                    player.wage = static_cast<long long>(player.wage * (1.05 + randInt(0, 15) / 100.0));
                    player.releaseClause = max(player.value * 2, player.wage * 45);
                    ++i;
                } else {
                    team_mgmt::detachPlayerFromSelections(*team, player.name);
                    team->players.erase(team->players.begin() + static_cast<long long>(i));
                }
            }
        }
    }
}

void updateManagerReputation(Career& career) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    if (rank > 0) {
        if (rank <= max(1, career.boardExpectedFinish - 1)) {
            career.managerReputation = clampInt(career.managerReputation + 2, 1, 100);
        } else if (rank > career.boardExpectedFinish + 2) {
            career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
        }
    }
    if ((career.myTeam->tactics == "Pressing" || career.myTeam->matchInstruction == "Juego directo") &&
        career.myTeam->goalsFor >= max(4, career.currentWeek * 2)) {
        career.managerReputation = clampInt(career.managerReputation + 1, 1, 100);
    }
    int promiseWarnings = 0;
    int youthContributors = 0;
    for (const auto& player : career.myTeam->players) {
        if (promiseAtRisk(player, career.currentWeek)) promiseWarnings++;
        if (player.age <= 21 && player.matchesPlayed >= 4) youthContributors++;
    }
    if (youthContributors >= 2) {
        career.managerReputation = clampInt(career.managerReputation + 1, 1, 100);
    }
    DressingRoomSnapshot dressing = dressing_room_service::buildSnapshot(*career.myTeam, career.currentWeek);
    if (dressing.socialTension <= 2 && career.myTeam->morale >= 68) {
        career.managerReputation = clampInt(career.managerReputation + 1, 1, 100);
    } else if (dressing.socialTension >= 5) {
        career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
    }
    if (career.boardConfidence <= 25 && promiseWarnings >= 2) {
        career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
    }
}

void handleManagerStatus(Career& career) {
    if (!career.myTeam) return;
    if (career.boardConfidence >= 20 && career.boardWarningWeeks < 6) return;
    emitUiMessage("");
    emitUiMessage("[Directiva] " + career.myTeam->name + " decide despedirte.");
    career.addNews(career.managerName + " fue despedido de " + career.myTeam->name + ".");
    career.managerReputation = clampInt(career.managerReputation - 8, 10, 100);

    vector<Team*> jobs = buildJobMarket(career, true);
    if (jobs.empty()) {
        for (auto& team : career.allTeams) {
            if (&team != career.myTeam) jobs.push_back(&team);
        }
    }
    if (jobs.empty()) return;

    emitUiMessage("Debes elegir nuevo club:");
    for (size_t i = 0; i < jobs.size(); ++i) {
        emitUiMessage(to_string(i + 1) + ". " + jobs[i]->name + " (" + divisionDisplay(jobs[i]->division) + ")");
    }

    int choice = 1;
    if (managerJobSelectionCallback()) {
        int selected = managerJobSelectionCallback()(career, jobs);
        if (selected >= 0 && selected < static_cast<int>(jobs.size())) {
            choice = selected + 1;
        }
    }
    takeManagerJob(career, jobs[static_cast<size_t>(choice - 1)], "Llega tras un despido reciente.");
}

vector<vector<pair<int, int>>> buildRoundRobinIndexSchedule(int teamCount, bool doubleRound) {
    vector<vector<pair<int, int>>> out;
    if (teamCount < 2) return out;
    vector<int> indices;
    indices.reserve(teamCount + 1);
    for (int i = 0; i < teamCount; ++i) indices.push_back(i);
    if (indices.size() % 2 == 1) indices.push_back(-1);

    int size = static_cast<int>(indices.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = indices[static_cast<size_t>(i)];
            int b = indices[static_cast<size_t>(size - 1 - i)];
            if (a == -1 || b == -1) continue;
            if (round % 2 == 0) matches.push_back({a, b});
            else matches.push_back({b, a});
        }
        out.push_back(matches);
        int last = indices.back();
        for (int i = size - 1; i > 1; --i) indices[static_cast<size_t>(i)] = indices[static_cast<size_t>(i - 1)];
        indices[1] = last;
    }

    if (doubleRound) {
        int base = static_cast<int>(out.size());
        for (int i = 0; i < base; ++i) {
            vector<pair<int, int>> reverseMatches;
            for (const auto& match : out[static_cast<size_t>(i)]) {
                reverseMatches.push_back({match.second, match.first});
            }
            out.push_back(reverseMatches);
        }
    }
    return out;
}

void simulateBackgroundDivisionWeek(Career& career, const string& divisionId) {
    if (divisionId.empty() || divisionId == career.activeDivision) return;
    vector<Team*> teams = career.getDivisionTeams(divisionId);
    if (teams.size() < 2) return;
    sort(teams.begin(), teams.end(), [](Team* left, Team* right) {
        if (left->tiebreakerSeed != right->tiebreakerSeed) return left->tiebreakerSeed < right->tiebreakerSeed;
        return left->name < right->name;
    });
    auto schedule = buildRoundRobinIndexSchedule(static_cast<int>(teams.size()), true);
    int round = career.currentWeek - 1;
    if (round < 0 || round >= static_cast<int>(schedule.size())) return;

    int headlineMargin = -1;
    string headline;
    for (const auto& match : schedule[static_cast<size_t>(round)]) {
        Team* home = teams[static_cast<size_t>(match.first)];
        Team* away = teams[static_cast<size_t>(match.second)];
        MatchResult result = playMatch(*home, *away, false, false);
        int margin = abs(result.homeGoals - result.awayGoals);
        if (margin > headlineMargin) {
            headlineMargin = margin;
            headline = home->name + " " + to_string(result.homeGoals) + "-" + to_string(result.awayGoals) + " " +
                       away->name;
        }
    }

    LeagueTable table;
    table.title = divisionDisplay(divisionId);
    table.ruleId = divisionId;
    for (Team* team : teams) table.addTeam(team);
    table.sortTable();
    Team* leader = table.teams.empty() ? nullptr : table.teams.front();
    Team* bottom = table.teams.empty() ? nullptr : table.teams.back();

    if (!headline.empty() && (career.currentWeek % 4 == 0 || headlineMargin >= 3)) {
        string news = "[Mundo] " + divisionDisplay(divisionId) + ": " + headline;
        if (leader) news += " | Lider " + leader->name + " (" + to_string(leader->points) + " pts)";
        career.addNews(news);
    }
    if (leader && randInt(1, 100) <= world_state_service::worldRuleValue("background_leader_story_chance", 14)) {
        career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": " + leader->name +
                       " instala una historia de temporada con estilo " + leader->clubStyle + ".");
    }
    if (bottom && randInt(1, 100) <= world_state_service::worldRuleValue("background_pressure_story_chance", 10)) {
        career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": crece la presion en " + bottom->name +
                       " por su mala racha.");
    }
    if (bottom && randInt(1, 100) <= world_state_service::worldRuleValue("background_manager_review_chance", 8)) {
        ensureTeamIdentity(*bottom);
        bottom->morale = clampInt(bottom->morale - 3, 0, 100);
        bottom->jobSecurity = clampInt(bottom->jobSecurity - 6, 5, 92);
        career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": " + bottom->name +
                       " entra en revision de banquillo tras otra semana bajo presion con " + bottom->headCoachName + ".");
    }
    if (leader && leader->youthFacilityLevel + leader->youthCoach >= 130 && randInt(1, 100) <= world_state_service::worldRuleValue("background_youth_promotion_chance", 12)) {
        const int maxSquad = getCompetitionConfig(divisionId).maxSquadSize;
        if (maxSquad <= 0 || static_cast<int>(leader->players.size()) < maxSquad) {
            const string youthPosition = leader->goalsAgainst > leader->goalsFor ? "DEF" : "MED";
            Player promoted = makeRandomPlayer(youthPosition,
                                               max(40, leader->getAverageSkill() - 18),
                                               max(leader->getAverageSkill() - 4, 45),
                                               17,
                                               19);
            promoted.potential = clampInt(promoted.skill + randInt(8, 16), promoted.skill, 95);
            promoted.contractWeeks = 156;
            promoted.wage = max(2500LL, promoted.wage / 2);
            leader->addPlayer(promoted);
            career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": " + leader->name +
                           " promociona al juvenil " + promoted.name + ".");
        }
    }
    if (leader && randInt(1, 100) <= world_state_service::worldRuleValue("background_injury_story_chance", 8)) {
        Player* keyPlayer = nullptr;
        for (auto& player : leader->players) {
            if (player.injured) continue;
            if (!keyPlayer || player.skill > keyPlayer->skill) keyPlayer = &player;
        }
        if (keyPlayer) {
            keyPlayer->injured = true;
            keyPlayer->injuryType = randInt(0, 1) == 0 ? "Sobrecarga" : "Muscular";
            keyPlayer->injuryWeeks = randInt(1, 3);
            keyPlayer->injuryHistory++;
            career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": " + leader->name +
                           " pierde por lesion a " + keyPlayer->name + " durante " +
                           to_string(keyPlayer->injuryWeeks) + " semana(s).");
        }
    }
}

void emitSeasonTransitionSummary(const SeasonTransitionSummary& summary) {
    emitUiMessage("");
    emitUiMessage("--- Cierre de Temporada ---");
    for (const string& line : summary.lines) {
        emitUiMessage(line);
    }
}


}  // namespace

void checkAchievements(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->wins >= 10 &&
        find(career.achievements.begin(), career.achievements.end(), "10 Victorias") == career.achievements.end()) {
        career.achievements.push_back("10 Victorias");
        emitUiMessage("Logro desbloqueado: 10 Victorias!");
    }
}

void simulateCareerWeek(Career& career) {
    if (career.activeTeams.empty() || career.schedule.empty()) {
        emitUiMessage("No hay calendario disponible.");
        return;
    }
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        emitSeasonTransitionSummary(endSeason(career));
        return;
    }

    emitUiMessage("");
    emitUiMessage("Simulando semana " + to_string(career.currentWeek) + "...");
    career.leagueTable.sortTable();
    LeagueTable northTable;
    LeagueTable southTable;
    bool useGroups = career.usesGroupFormat();
    if (useGroups) {
        northTable = buildCompetitionGroupTable(career, true);
        southTable = buildCompetitionGroupTable(career, false);
    }

    const auto& matches = career.schedule[static_cast<size_t>(career.currentWeek - 1)];
    vector<vector<int>> suspensionsBefore;
    suspensionsBefore.reserve(career.activeTeams.size());
    for (auto* team : career.activeTeams) {
        vector<int> snapshot;
        snapshot.reserve(team->players.size());
        for (const auto& player : team->players) snapshot.push_back(player.matchesSuspended);
        suspensionsBefore.push_back(snapshot);
    }

    vector<int> pointsBefore;
    pointsBefore.reserve(career.activeTeams.size());
    for (auto* team : career.activeTeams) pointsBefore.push_back(team->points);

    int myTeamPointsDelta = 0;
    for (const auto& match : matches) {
        Team* home = career.activeTeams[static_cast<size_t>(match.first)];
        Team* away = career.activeTeams[static_cast<size_t>(match.second)];
        bool verbose = (home == career.myTeam || away == career.myTeam);
        team_ai::adjustCpuTactics(*home, *away, career.myTeam);
        team_ai::adjustCpuTactics(*away, *home, career.myTeam);

        bool key = false;
        if (useGroups) {
            int homeGroup = competitionGroupForTeam(career, home);
            int awayGroup = competitionGroupForTeam(career, away);
            if (homeGroup == awayGroup && homeGroup == 0) {
                key = isKeyMatch(northTable, home, away);
            } else if (homeGroup == awayGroup && homeGroup == 1) {
                key = isKeyMatch(southTable, home, away);
            } else {
                key = isKeyMatch(career.leagueTable, home, away);
            }
        } else {
            key = isKeyMatch(career.leagueTable, home, away);
        }
        if (verbose && key) emitUiMessage("[Aviso] Partido clave de la semana.");

        MatchResult result = playMatch(*home, *away, verbose, key);
        storeMatchAnalysis(career, *home, *away, result, false);
    }

    for (const auto& division : career.divisions) {
        simulateBackgroundDivisionWeek(career, division.id);
    }

    bool cupWeek = career.cupActive &&
                   (career.currentWeek == 1 || career.currentWeek % 4 == 0 ||
                    career.currentWeek == static_cast<int>(career.schedule.size()));
    if (cupWeek) simulateSeasonCupRound(career);

    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        const auto& snapshot = suspensionsBefore[i];
        size_t limit = min(snapshot.size(), team->players.size());
        for (size_t j = 0; j < limit; ++j) {
            if (snapshot[j] > 0 && team->players[j].matchesSuspended > 0) {
                team->players[j].matchesSuspended--;
            }
        }
    }

    for (auto& team : career.allTeams) {
        healInjuries(team, false);
        recoverFitness(team, 7);
        const bool congestedTraining = cupWeek ? team.division == career.activeDivision : (career.currentWeek % 5 == 0 && team.division != career.activeDivision);
        player_dev::applyWeeklyTrainingPlan(team, congestedTraining);
    }

    updateContracts(career);
    processIncomingOffers(career);
    transfer_market::processCpuTransfers(career);
    transfer_market::processLoanReturns(career);
    applyWeeklyFinances(career, pointsBefore);
    career.leagueTable.sortTable();

    if (career.myTeam) {
        for (size_t i = 0; i < career.activeTeams.size(); ++i) {
            if (career.activeTeams[i] == career.myTeam) {
                myTeamPointsDelta = career.myTeam->points - pointsBefore[i];
                break;
            }
        }
        if (career.boardMonthlyObjective.find("puntos") != string::npos) {
            career.boardMonthlyProgress += myTeamPointsDelta;
        }
        updateSquadDynamics(career, myTeamPointsDelta);
        WorldPulseSummary worldPulse = world_state_service::processWeeklyWorldState(career);
        for (size_t i = 0; i < worldPulse.headlines.size() && i < 2; ++i) {
            emitUiMessage("[Mundo] " + worldPulse.headlines[i]);
        }
    }

    runMonthlyDevelopment(career);
    updateShortlistAlerts(career);
    career.updateDynamicObjectiveStatus();
    career.updateBoardConfidence();
    updateManagerReputation(career);
    if (career.myTeam) ensureTeamIdentity(*career.myTeam);
    weeklyDashboard(career);
    applyClubEvent(career);

    if (career.myTeam) {
        if (myTeamPointsDelta == 3) {
            career.addNews(career.myTeam->name + " gana en la fecha " + to_string(career.currentWeek) + ".");
        } else if (myTeamPointsDelta == 1) {
            career.addNews(career.myTeam->name + " empata en la fecha " + to_string(career.currentWeek) + ".");
        } else {
            career.addNews(career.myTeam->name + " pierde en la fecha " + to_string(career.currentWeek) + ".");
        }
        if (career.boardWarningWeeks >= 4) {
            career.addNews("La directiva aumenta la presion sobre " + career.myTeam->name + ".");
        }
        generateManagerCareerEvents(career);
        generateWeeklyNarratives(career, myTeamPointsDelta);
    }

    handleManagerStatus(career);
    career.currentWeek++;
    checkAchievements(career);
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        emitSeasonTransitionSummary(endSeason(career));
    }
}
