#include "models.h"

#include "competition/competition.h"
#include "io/io.h"
#include "utils/utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

static constexpr int kCareerSaveVersion = 5;

void applyPositionStats(Player& p) {
    string norm = normalizePosition(p.position);
    if (norm == "ARQ") {
        p.attack = clampInt(p.skill - 30, 10, 70);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "DEF") {
        p.attack = clampInt(p.skill - 10, 20, 90);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "MED") {
        p.attack = clampInt(p.skill, 25, 99);
        p.defense = clampInt(p.skill, 25, 99);
    } else if (norm == "DEL") {
        p.attack = clampInt(p.skill + 10, 30, 99);
        p.defense = clampInt(p.skill - 10, 20, 90);
    }
}

string defaultRoleForPosition(const string& position) {
    string norm = normalizePosition(position);
    if (norm == "ARQ") return "Tradicional";
    if (norm == "DEF") return "Stopper";
    if (norm == "MED") return "BoxToBox";
    if (norm == "DEL") return "Poacher";
    return "Default";
}

string defaultDevelopmentPlanForPosition(const string& position) {
    string norm = normalizePosition(position);
    if (norm == "ARQ") return "Reflejos";
    if (norm == "DEF") return "Defensa";
    if (norm == "MED") return "Creatividad";
    if (norm == "DEL") return "Finalizacion";
    return "Equilibrado";
}

static string randomPreferredFoot() {
    int roll = randInt(1, 100);
    if (roll <= 68) return "Derecho";
    if (roll <= 92) return "Izquierdo";
    return "Ambos";
}

static void addUniquePosition(vector<string>& positions, const string& value) {
    string normalized = normalizePosition(value);
    if (normalized == "N/A") return;
    if (find(positions.begin(), positions.end(), normalized) == positions.end()) {
        positions.push_back(normalized);
    }
}

static vector<string> inferSecondaryPositions(const Player& p) {
    vector<string> positions;
    string primary = normalizePosition(p.position);
    if (primary == "ARQ") return positions;

    if (primary == "DEF") {
        if (p.versatility >= 48 || p.attack >= p.defense - 6 || p.stamina >= 68) addUniquePosition(positions, "MED");
    } else if (primary == "MED") {
        if (p.defense >= p.attack - 4 || p.tacticalDiscipline >= 64) addUniquePosition(positions, "DEF");
        if (p.attack >= p.defense - 2 || p.bigMatches >= 62) addUniquePosition(positions, "DEL");
    } else if (primary == "DEL") {
        if (p.versatility >= 42 || p.stamina >= 66) addUniquePosition(positions, "MED");
    }

    if (p.versatility >= 76) {
        if (primary != "DEF") addUniquePosition(positions, "DEF");
        if (primary != "MED") addUniquePosition(positions, "MED");
        if (primary != "DEL") addUniquePosition(positions, "DEL");
    }
    return positions;
}

void ensurePlayerProfile(Player& p, bool regenerateTraits) {
    string normalizedPos = normalizePosition(p.position);
    if (normalizedPos == "N/A") normalizedPos = "MED";
    p.position = normalizedPos;
    if (p.preferredFoot.empty()) p.preferredFoot = randomPreferredFoot();
    if (p.consistency <= 0) {
        int base = 38 + p.professionalism / 3 + p.happiness / 8 + randInt(-6, 8);
        p.consistency = clampInt(base, 1, 99);
    }
    if (p.bigMatches <= 0) {
        int base = 34 + p.ambition / 3 + p.leadership / 6 + randInt(-8, 9);
        p.bigMatches = clampInt(base, 1, 99);
    }
    if (p.currentForm <= 0) {
        int base = 44 + p.skill / 5 + randInt(-10, 10);
        p.currentForm = clampInt(base, 1, 99);
    }
    if (p.tacticalDiscipline <= 0) {
        int base = 36 + p.professionalism / 2 + p.chemistry / 6 + randInt(-8, 8);
        p.tacticalDiscipline = clampInt(base, 1, 99);
    }
    if (p.versatility <= 0) {
        int base = 30 + p.stamina / 4 + p.professionalism / 5 + randInt(-8, 12);
        p.versatility = clampInt(base, 1, 99);
    }
    if (p.secondaryPositions.empty()) p.secondaryPositions = inferSecondaryPositions(p);
    if (p.role.empty()) p.role = defaultRoleForPosition(p.position);
    if (p.developmentPlan.empty()) p.developmentPlan = defaultDevelopmentPlanForPosition(p.position);
    if (p.promisedRole.empty()) p.promisedRole = "Sin promesa";
    if (regenerateTraits || p.traits.empty()) p.traits = generatePlayerTraits(p, p.age <= 19);
    if (p.traits.empty()) p.traits = generatePlayerTraits(p, p.age <= 19);
}

int positionFitScore(const Player& p, const string& desiredPosition) {
    string desired = normalizePosition(desiredPosition);
    string primary = normalizePosition(p.position);
    if (desired == "N/A" || desired.empty()) return 60;
    if (primary == desired) return 100 + p.versatility / 8;
    if (find(p.secondaryPositions.begin(), p.secondaryPositions.end(), desired) != p.secondaryPositions.end()) {
        return 78 + p.versatility / 4;
    }
    if (primary == "MED" && (desired == "DEF" || desired == "DEL")) return 45 + p.versatility / 5;
    if (primary == "DEF" && desired == "MED") return 42 + p.versatility / 6;
    if (primary == "DEL" && desired == "MED") return 40 + p.versatility / 6;
    if (primary == "ARQ" || desired == "ARQ") return 6;
    return 18 + p.versatility / 8;
}

string playerReliabilityLabel(const Player& p) {
    int score = (p.consistency * 2 + p.professionalism) / 3;
    if (score >= 78) return "Muy fiable";
    if (score >= 62) return "Solido";
    if (score >= 48) return "Variable";
    return "Irregular";
}

string playerFormLabel(const Player& p) {
    if (p.currentForm >= 76) return "En racha";
    if (p.currentForm >= 60) return "Bien";
    if (p.currentForm >= 45) return "Normal";
    return "Baja";
}

static int divisionPrestigeBase(const string& division) {
    string id = toLower(trim(division));
    if (id == "primera division") return 68;
    if (id == "primera b") return 56;
    if (id == "segunda division") return 48;
    if (id == "tercera a") return 40;
    if (id == "tercera b") return 32;
    return 36;
}

static string inferClubStyle(const Team& team) {
    if (team.tactics == "Pressing" || team.pressingIntensity >= 4) return "Presion vertical";
    if (team.matchInstruction == "Juego directo" || team.tempo >= 4) return "Transicion directa";
    if (team.matchInstruction == "Por bandas" || team.width >= 4) return "Ataque por bandas";
    if (team.tactics == "Defensive" || team.matchInstruction == "Bloque bajo" || team.defensiveLine <= 2) {
        return "Bloque ordenado";
    }
    if (team.trainingFacilityLevel >= 3 || team.formation.find("4-3-3") != string::npos) {
        return "Control de posesion";
    }
    return "Equilibrio competitivo";
}

static string inferYouthIdentity(const Team& team) {
    if (team.youthFacilityLevel >= 4 || team.youthCoach >= 75) return "Cantera estructurada";
    if (team.youthFacilityLevel >= 3 || team.trainingFacilityLevel >= 3) return "Desarrollo mixto";
    if (team.fanBase >= 28 && team.clubPrestige >= 60) return "Plantel de mercado";
    return "Talento local";
}

static string inferPrimaryRival(const string& clubName) {
    string id = normalizeTeamId(clubName);
    static const vector<pair<string, string>> rivals = {
        {"colocolo", "Universidad de Chile"},
        {"universidaddechile", "Colo-Colo"},
        {"universidadcatolica", "Universidad de Chile"},
        {"deportesiquique", "San Marcos de Arica"},
        {"sanmarcosdearica", "Deportes Iquique"},
        {"cobreloa", "Deportes Antofagasta"},
        {"deportesantofagasta", "Cobreloa"},
        {"santiagowanderers", "Everton"},
        {"everton", "Santiago Wanderers"},
        {"unionespanola", "Palestino"},
        {"palestino", "Union Espanola"},
        {"magallanes", "Santiago Morning"},
        {"santiagomorning", "Magallanes"},
        {"rangers", "Curico Unido"},
        {"curicounido", "Rangers"},
        {"deportesconcepcion", "Fernandez Vial"},
        {"fernandezvial", "Deportes Concepcion"}
    };
    for (const auto& rival : rivals) {
        if (rival.first == id) return rival.second;
    }
    return "";
}

static int calculateTeamPrestige(const Team& team) {
    long long achievementScore = static_cast<long long>(team.achievements.size()) * 2;
    long long financialPower = clampInt(static_cast<int>(team.sponsorWeekly / 60000LL), 0, 10);
    long long facilities = (team.stadiumLevel - 1) * 3 + (team.youthFacilityLevel - 1) * 3 +
                           (team.trainingFacilityLevel - 1) * 3;
    long long crowd = team.fanBase / 3;
    long long debtPenalty = clampInt(static_cast<int>(team.debt / 250000LL), 0, 14);
    int prestige = divisionPrestigeBase(team.division) + static_cast<int>(achievementScore + financialPower + facilities + crowd) -
                   static_cast<int>(debtPenalty);
    return clampInt(prestige, 20, 95);
}

int teamPrestigeScore(const Team& team) {
    return calculateTeamPrestige(team);
}

void ensureTeamIdentity(Team& team) {
    team.clubPrestige = calculateTeamPrestige(team);
    team.clubStyle = inferClubStyle(team);
    team.youthIdentity = inferYouthIdentity(team);
    if (team.primaryRival.empty()) team.primaryRival = inferPrimaryRival(team.name);
}

bool areRivalClubs(const Team& a, const Team& b) {
    string aName = normalizeTeamId(a.name);
    string bName = normalizeTeamId(b.name);
    if (!a.primaryRival.empty() && normalizeTeamId(a.primaryRival) == bName) return true;
    if (!b.primaryRival.empty() && normalizeTeamId(b.primaryRival) == aName) return true;
    return false;
}

string teamExpectationLabel(const Team& team) {
    int prestige = teamPrestigeScore(team);
    if (prestige >= 74) return "Pelear arriba";
    if (prestige >= 58) return "Competir por playoffs";
    if (prestige >= 44) return "Mitad de tabla";
    return "Evitar el fondo";
}

bool playerHasTrait(const Player& p, const string& trait) {
    return find(p.traits.begin(), p.traits.end(), trait) != p.traits.end();
}

string joinStringValues(const vector<string>& values, const string& separator) {
    string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) out += separator;
        out += values[i];
    }
    return out;
}

vector<string> generatePlayerTraits(const Player& p, bool youth) {
    vector<string> traits;
    auto addTrait = [&](const string& trait) {
        if (find(traits.begin(), traits.end(), trait) == traits.end()) {
            traits.push_back(trait);
        }
    };

    string pos = normalizePosition(p.position);
    if (p.leadership >= 72 || p.professionalism >= 80) addTrait("Lider");
    if (p.professionalism >= 76 && p.ambition >= 62 && traits.size() < 3) addTrait("Competidor");
    if ((p.injuryHistory >= 2 || p.stamina <= 60 || p.age >= 31) && traits.size() < 3) addTrait("Fragil");
    if ((p.ambition >= 75 && p.professionalism <= 48) && traits.size() < 3) addTrait("Caliente");
    if (p.bigMatches >= 74 && traits.size() < 3) addTrait("Cita grande");
    if (p.consistency <= 45 && traits.size() < 3) addTrait("Irregular");
    if (p.versatility >= 76 && traits.size() < 3) addTrait("Versatil");
    if (p.preferredFoot == "Ambos" && traits.size() < 3) addTrait("Dos perfiles");
    if (pos == "MED" && p.attack >= p.defense && traits.size() < 3) addTrait("Pase riesgoso");
    if ((pos == "MED" || pos == "DEF") && p.stamina >= 72 && traits.size() < 3) addTrait("Llega al area");
    if ((pos == "DEF" || pos == "DEL") && p.stamina >= 74 && p.professionalism >= 55 && traits.size() < 3) addTrait("Presiona");
    if ((pos == "ARQ" || pos == "DEF") && p.defense >= p.attack + 8 && traits.size() < 3) addTrait("Muralla");
    if (youth && p.potential - p.skill >= 12 && traits.size() < 3) addTrait("Proyecto");
    if (traits.empty()) addTrait("Equilibrado");
    if (traits.size() > 3) traits.resize(3);
    return traits;
}

Player makeRandomPlayer(const string& position, int skillMin, int skillMax, int ageMin, int ageMax) {
    Player p;
    p.name = "Jugador" + to_string(randInt(1000, 9999));
    p.position = position;
    p.skill = randInt(skillMin, skillMax);
    p.potential = clampInt(p.skill + randInt(0, 12), p.skill, 95);
    p.age = randInt(ageMin, ageMax);
    p.stamina = clampInt(p.skill + randInt(-5, 5), 30, 99);
    p.fitness = p.stamina;
    if (position == "ARQ") {
        p.attack = clampInt(p.skill - 30, 10, 70);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (position == "DEF") {
        p.attack = clampInt(p.skill - 10, 20, 90);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (position == "MED") {
        p.attack = clampInt(p.skill, 25, 99);
        p.defense = clampInt(p.skill, 25, 99);
    } else {
        p.attack = clampInt(p.skill + 10, 30, 99);
        p.defense = clampInt(p.skill - 10, 20, 90);
    }
    p.value = static_cast<long long>(p.skill) * 10000;
    p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 800);
    p.releaseClause = max(50000LL, p.value * (18 + randInt(0, 8)) / 10);
    p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 25, 99);
    p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    p.desiredStarts = (p.skill >= skillMax - 5) ? 3 : 1;
    p.startsThisSeason = 0;
    p.wantsToLeave = false;
    p.onLoan = false;
    p.parentClub.clear();
    p.loanWeeksRemaining = 0;
    p.contractWeeks = randInt(52, 156);
    p.injured = false;
    p.injuryType = "";
    p.injuryWeeks = 0;
    p.injuryHistory = 0;
    p.yellowAccumulation = 0;
    p.seasonYellowCards = 0;
    p.seasonRedCards = 0;
    p.matchesSuspended = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    p.lastTrainedSeason = -1;
    p.lastTrainedWeek = -1;
    p.role = defaultRoleForPosition(position);
    ensurePlayerProfile(p, true);
    return p;
}

Team::Team(string n)
    : name(std::move(n)),
      division(""),
      tactics("Balanced"),
      formation("4-4-2"),
      budget(0),
      morale(50),
      trainingFocus("Balanceado"),
      points(0),
      goalsFor(0),
      goalsAgainst(0),
      awayGoals(0),
      wins(0),
      draws(0),
      losses(0),
      yellowCards(0),
      redCards(0),
      tiebreakerSeed(0),
      pressingIntensity(3),
      defensiveLine(3),
      tempo(3),
      width(3),
      markingStyle("Zonal"),
      preferredBench(),
      captain(""),
      penaltyTaker(""),
      freeKickTaker(""),
      cornerTaker(""),
      rotationPolicy("Balanceado"),
      assistantCoach(55),
      fitnessCoach(55),
      scoutingChief(55),
      youthCoach(55),
      medicalTeam(55),
      youthRegion("Metropolitana"),
      debt(0),
      sponsorWeekly(25000),
      stadiumLevel(1),
      youthFacilityLevel(1),
      trainingFacilityLevel(1),
      fanBase(12),
      clubPrestige(0),
      clubStyle(""),
      youthIdentity(""),
      primaryRival(""),
      matchInstruction("Equilibrado") {}

void Team::addPlayer(const Player& p) {
    players.push_back(p);
}

void Team::addHeadToHeadPoints(const string& opponent, int pointsToAdd) {
    for (auto& record : headToHead) {
        if (record.opponent == opponent) {
            record.points += pointsToAdd;
            return;
        }
    }
    headToHead.push_back({opponent, pointsToAdd});
}

int Team::headToHeadPointsAgainst(const string& opponent) const {
    for (const auto& record : headToHead) {
        if (record.opponent == opponent) return record.points;
    }
    return 0;
}

void Team::resetSeasonStats() {
    points = 0;
    goalsFor = 0;
    goalsAgainst = 0;
    awayGoals = 0;
    wins = 0;
    draws = 0;
    losses = 0;
    yellowCards = 0;
    redCards = 0;
    tiebreakerSeed = randInt(0, 1000000);
    headToHead.clear();
}

static int goalDifference(const Team* team) {
    return team->goalsFor - team->goalsAgainst;
}

static bool compareTerceraABase(const Team* a, const Team* b) {
    if (a->points != b->points) return a->points > b->points;
    int aGD = goalDifference(a);
    int bGD = goalDifference(b);
    if (aGD != bGD) return aGD > bGD;
    if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
    if (a->wins != b->wins) return a->wins > b->wins;
    if (a->awayGoals != b->awayGoals) return a->awayGoals > b->awayGoals;
    return a->name < b->name;
}

static bool equalTerceraABase(const Team* a, const Team* b) {
    return a->points == b->points &&
           goalDifference(a) == goalDifference(b) &&
           a->goalsFor == b->goalsFor &&
           a->wins == b->wins &&
           a->awayGoals == b->awayGoals;
}

static bool compareTerceraBBase(const Team* a, const Team* b) {
    if (a->points != b->points) return a->points > b->points;
    int aGD = goalDifference(a);
    int bGD = goalDifference(b);
    if (aGD != bGD) return aGD > bGD;
    if (a->wins != b->wins) return a->wins > b->wins;
    if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
    if (a->awayGoals != b->awayGoals) return a->awayGoals > b->awayGoals;
    return a->name < b->name;
}

static bool equalTerceraBBase(const Team* a, const Team* b) {
    return a->points == b->points &&
           goalDifference(a) == goalDifference(b) &&
           a->wins == b->wins &&
           a->goalsFor == b->goalsFor &&
           a->awayGoals == b->awayGoals;
}

static int headToHeadPointsWithinTie(const Team* team, const vector<Team*>& tiedTeams) {
    int total = 0;
    for (auto* other : tiedTeams) {
        if (other != team) total += team->headToHeadPointsAgainst(other->name);
    }
    return total;
}

static void applyHeadToHeadOrder(vector<Team*>& teams, bool terceraA) {
    auto equalBase = terceraA ? equalTerceraABase : equalTerceraBBase;
    size_t start = 0;
    while (start < teams.size()) {
        size_t end = start + 1;
        while (end < teams.size() && equalBase(teams[start], teams[end])) {
            end++;
        }
        if (end - start > 1) {
            vector<Team*> tiedTeams(teams.begin() + static_cast<long long>(start),
                                    teams.begin() + static_cast<long long>(end));
            stable_sort(teams.begin() + static_cast<long long>(start),
                        teams.begin() + static_cast<long long>(end),
                        [&](Team* a, Team* b) {
                            int aH2H = headToHeadPointsWithinTie(a, tiedTeams);
                            int bH2H = headToHeadPointsWithinTie(b, tiedTeams);
                            if (aH2H != bH2H) return aH2H > bH2H;
                            if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
                            return a->name < b->name;
                        });
        }
        start = end;
    }
}

static void parseFormation(const string& formation, int& defCount, int& midCount, int& fwdCount) {
    defCount = 4;
    midCount = 4;
    fwdCount = 2;
    string s = trim(formation);
    if (s.empty()) return;
    stringstream ss(s);
    string token;
    vector<int> nums;
    while (getline(ss, token, '-')) {
        token = trim(token);
        if (token.empty()) continue;
        try {
            nums.push_back(stoi(token));
        } catch (...) {
        }
    }
    if (nums.size() == 3) {
        defCount = nums[0];
        midCount = nums[1];
        fwdCount = nums[2];
    }
    int total = 1 + defCount + midCount + fwdCount;
    if (total != 11) {
        defCount = 4;
        midCount = 4;
        fwdCount = 2;
    }
}

static bool isAvailableForSelection(const Player& player) {
    return !player.injured && player.matchesSuspended <= 0;
}

static int selectionScore(const Team& team, int idx) {
    const Player& player = team.players[idx];
    int score = player.skill * 10 + player.fitness * 4 + player.currentForm * 3 + player.consistency * 2 +
                player.tacticalDiscipline - player.matchesPlayed;
    if (team.rotationPolicy == "Titulares") {
        score = player.skill * 12 + player.fitness * 2 + player.currentForm * 4 + player.consistency * 2 -
                player.matchesPlayed / 2;
    } else if (team.rotationPolicy == "Rotacion") {
        score = player.skill * 8 + player.fitness * 6 + player.currentForm * 2 + player.versatility -
                player.matchesPlayed * 2;
    }
    if (player.wantsToLeave) score -= 12;
    if (!team.captain.empty() && player.name == team.captain) score += 10;
    if (!team.penaltyTaker.empty() && player.name == team.penaltyTaker) score += 6;
    return score;
}

vector<int> Team::getStartingXIIndices() const {
    vector<int> xi;
    if (players.empty()) return xi;
    int defCount, midCount, fwdCount;
    parseFormation(formation, defCount, midCount, fwdCount);

    vector<bool> used(players.size(), false);
    auto tryPreferred = [&]() {
        for (const auto& preferred : preferredXI) {
            for (size_t i = 0; i < players.size(); ++i) {
                if (used[i]) continue;
                if (players[i].name != preferred) continue;
                if (!isAvailableForSelection(players[i])) continue;
                used[i] = true;
                xi.push_back(static_cast<int>(i));
                break;
            }
            if (xi.size() >= 11) return;
        }
    };
    auto pick = [&](const string& pos, int count) {
        if (xi.size() >= 11 || count <= 0) return;
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (used[i]) continue;
            if (!isAvailableForSelection(players[i])) continue;
            if (positionFitScore(players[i], pos) >= 40) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            int aScore = selectionScore(*this, a) + positionFitScore(players[a], pos) * 6;
            int bScore = selectionScore(*this, b) + positionFitScore(players[b], pos) * 6;
            if (aScore != bScore) return aScore > bScore;
            return players[a].skill > players[b].skill;
        });
        for (int i = 0; i < count && i < static_cast<int>(candidates.size()); ++i) {
            int idx = candidates[i];
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    };

    tryPreferred();
    pick("ARQ", 1);
    pick("DEF", defCount);
    pick("MED", midCount);
    pick("DEL", fwdCount);

    if (xi.size() < 11) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (!used[i] && isAvailableForSelection(players[i])) {
                candidates.push_back(static_cast<int>(i));
            }
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            int aScore = selectionScore(*this, a);
            int bScore = selectionScore(*this, b);
            if (aScore != bScore) return aScore > bScore;
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    }
    if (xi.size() < 11) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (!used[i] && players[i].matchesSuspended <= 0) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            int aScore = selectionScore(*this, a);
            int bScore = selectionScore(*this, b);
            if (aScore != bScore) return aScore > bScore;
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    }
    if (xi.size() < 11) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (!used[i]) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            int aScore = selectionScore(*this, a);
            int bScore = selectionScore(*this, b);
            if (aScore != bScore) return aScore > bScore;
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    }
    return xi;
}

vector<int> Team::getBenchIndices(int count) const {
    vector<int> xi = getStartingXIIndices();
    vector<int> bench;
    if (count <= 0) return bench;
    vector<bool> used(players.size(), false);
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(players.size())) used[idx] = true;
    }

    auto tryPreferred = [&]() {
        for (const auto& preferred : preferredBench) {
            for (size_t i = 0; i < players.size(); ++i) {
                if (used[i]) continue;
                if (players[i].name != preferred) continue;
                if (!isAvailableForSelection(players[i])) continue;
                used[i] = true;
                bench.push_back(static_cast<int>(i));
                break;
            }
            if (static_cast<int>(bench.size()) >= count) return;
        }
    };

    tryPreferred();
    if (static_cast<int>(bench.size()) < count) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (used[i]) continue;
            if (!isAvailableForSelection(players[i])) continue;
            candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            int aScore = selectionScore(*this, a);
            int bScore = selectionScore(*this, b);
            if (aScore != bScore) return aScore > bScore;
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            bench.push_back(idx);
            if (static_cast<int>(bench.size()) >= count) break;
        }
    }
    return bench;
}

int Team::getTotalAttack() const {
    auto xi = getStartingXIIndices();
    int total = 0;
    for (int idx : xi) total += players[idx].attack;
    return total;
}

int Team::getTotalDefense() const {
    auto xi = getStartingXIIndices();
    int total = 0;
    for (int idx : xi) total += players[idx].defense;
    return total;
}

int Team::getAverageSkill() const {
    auto xi = getStartingXIIndices();
    if (xi.empty()) return 0;
    int total = 0;
    for (int idx : xi) total += players[idx].skill;
    return total / static_cast<int>(xi.size());
}

int Team::getAverageStamina() const {
    auto xi = getStartingXIIndices();
    if (xi.empty()) return 0;
    int total = 0;
    for (int idx : xi) total += players[idx].fitness;
    return total / static_cast<int>(xi.size());
}

long long Team::getSquadValue() const {
    long long total = 0;
    for (const auto& p : players) total += p.value;
    return total;
}

void LeagueTable::clear() {
    teams.clear();
}

void LeagueTable::addTeam(Team* team) {
    teams.push_back(team);
}

void LeagueTable::sortTable() {
    const CompetitionConfig& config = getCompetitionConfig(ruleId);
    if (config.tableProfile == CompetitionTableProfile::PrimeraLike) {
        sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
            if (a->points != b->points) return a->points > b->points;
            int aGD = goalDifference(a);
            int bGD = goalDifference(b);
            if (aGD != bGD) return aGD > bGD;
            if (a->wins != b->wins) return a->wins > b->wins;
            if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
            if (a->awayGoals != b->awayGoals) return a->awayGoals > b->awayGoals;
            if (a->redCards != b->redCards) return a->redCards < b->redCards;
            if (a->yellowCards != b->yellowCards) return a->yellowCards < b->yellowCards;
            if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
            return a->name < b->name;
        });
        return;
    }
    if (config.tableProfile == CompetitionTableProfile::TerceraA) {
        sort(teams.begin(), teams.end(), compareTerceraABase);
        applyHeadToHeadOrder(teams, true);
        return;
    }
    if (config.tableProfile == CompetitionTableProfile::TerceraB) {
        sort(teams.begin(), teams.end(), compareTerceraBBase);
        applyHeadToHeadOrder(teams, false);
        return;
    }
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        if (a->points != b->points) return a->points > b->points;
        int aGD = goalDifference(a);
        int bGD = goalDifference(b);
        if (aGD != bGD) return aGD > bGD;
        if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
        return a->name < b->name;
    });
}

vector<string> LeagueTable::formatLines() const {
    vector<string> lines;
    lines.push_back("");
    lines.push_back("--- Tabla " + (title.empty() ? string("Liga") : title) + " ---");
    lines.push_back("Pos | Equipo                 | Pts | PJ | G  | E  | P  | GF | GA | DG");
    lines.push_back("----+------------------------+-----+----+----+----+----+----+----+----");
    for (size_t i = 0; i < teams.size(); ++i) {
        Team* team = teams[i];
        int gd = team->goalsFor - team->goalsAgainst;
        int pj = team->wins + team->draws + team->losses;
        ostringstream line;
        line << setw(3) << i + 1 << " | " << setw(22) << left << team->name.substr(0, 22)
             << " | " << setw(3) << team->points
             << " | " << setw(2) << pj
             << " | " << setw(2) << team->wins
             << " | " << setw(2) << team->draws
             << " | " << setw(2) << team->losses
             << " | " << setw(2) << team->goalsFor
             << " | " << setw(2) << team->goalsAgainst
             << " | " << setw(2) << gd;
        lines.push_back(line.str());
    }
    return lines;
}

void LeagueTable::displayTable() {
    for (const auto& line : formatLines()) {
        cout << line << endl;
    }
}

const vector<DivisionInfo> kDivisions = {
    {"primera division", "data/LigaChilena/primera division", "Primera Division"},
    {"primera b", "data/LigaChilena/primera b", "Primera B"},
    {"segunda division", "data/LigaChilena/segunda division", "Segunda Division"},
    {"tercera division a", "data/LigaChilena/tercera division a", "Tercera Division A"},
    {"tercera division b", "data/LigaChilena/tercera division b", "Tercera Division B"}
};

Career::Career()
    : myTeam(nullptr),
      currentSeason(1),
      currentWeek(1),
      saveFile("saves/career_save.txt"),
      managerName("Manager"),
      managerReputation(50),
      boardConfidence(60),
      boardExpectedFinish(0),
      boardBudgetTarget(0),
      boardYouthTarget(1),
      boardWarningWeeks(0),
      boardMonthlyTarget(0),
      boardMonthlyProgress(0),
      boardMonthlyDeadlineWeek(0),
      cupActive(false),
      cupRound(0),
      initialized(false) {}

bool Career::usesSegundaFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::SegundaGroups &&
           (config.expectedTeamCount <= 0 || static_cast<int>(activeTeams.size()) == config.expectedTeamCount);
}

bool Career::usesTerceraBFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::TerceraB &&
           (config.expectedTeamCount <= 0 || static_cast<int>(activeTeams.size()) == config.expectedTeamCount);
}

bool Career::usesGroupFormat() const {
    return competitionUsesGroupStage(activeDivision, static_cast<int>(activeTeams.size()));
}

void Career::buildSegundaGroups() {
    buildRegionalGroups();
}

void Career::buildRegionalGroups() {
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    if (!usesGroupFormat()) return;
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    int split = config.groups.groupSize;
    if (split <= 0) return;
    for (int i = 0; i < static_cast<int>(activeTeams.size()); ++i) {
        if (i < split) groupNorthIdx.push_back(i);
        else groupSouthIdx.push_back(i);
    }
}

void Career::initializeLeague(bool forceReload) {
    if (initialized && !forceReload) return;
    myTeam = nullptr;
    allTeams.clear();
    activeTeams.clear();
    schedule.clear();
    leagueTable.clear();
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    divisions.clear();
    activeDivision.clear();
    loadWarnings.clear();

    for (const auto& div : kDivisions) {
        if (!isDirectory(div.folder)) continue;
        DivisionLoadResult divisionLoad = loadDivisionFromFolder(div.folder, div.id, allTeams);
        loadWarnings.insert(loadWarnings.end(), divisionLoad.warnings.begin(), divisionLoad.warnings.end());
        if (!divisionLoad.teams.empty()) {
            divisions.push_back(div);
        }
    }
    for (auto& team : allTeams) ensureTeamIdentity(team);
    initialized = true;
}

vector<Team*> Career::getDivisionTeams(const string& id) {
    vector<Team*> out;
    for (auto& team : allTeams) {
        if (team.division == id) out.push_back(&team);
    }
    return out;
}

static vector<vector<pair<int, int>>> buildRoundRobinSchedule(const vector<int>& teamIdx, bool doubleRound) {
    vector<vector<pair<int, int>>> out;
    if (teamIdx.size() < 2) return out;

    vector<int> idx = teamIdx;
    if (idx.size() % 2 == 1) idx.push_back(-1);

    int size = static_cast<int>(idx.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = idx[i];
            int b = idx[size - 1 - i];
            if (a == -1 || b == -1) continue;
            if (round % 2 == 0) matches.push_back({a, b});
            else matches.push_back({b, a});
        }
        out.push_back(matches);
        int last = idx.back();
        for (int i = size - 1; i > 1; --i) idx[i] = idx[i - 1];
        idx[1] = last;
    }

    if (doubleRound) {
        int base = static_cast<int>(out.size());
        for (int i = 0; i < base; ++i) {
            vector<pair<int, int>> rev;
            for (auto& m : out[i]) rev.push_back({m.second, m.first});
            out.push_back(rev);
        }
    }
    return out;
}

void Career::buildSchedule() {
    schedule.clear();
    int n = static_cast<int>(activeTeams.size());
    if (n < 2) return;

    if (usesGroupFormat()) {
        const CompetitionConfig& config = getCompetitionConfig(activeDivision);
        if (groupNorthIdx.empty() || groupSouthIdx.empty()) buildRegionalGroups();
        auto north = buildRoundRobinSchedule(groupNorthIdx, config.groups.doubleRoundRobin);
        auto south = buildRoundRobinSchedule(groupSouthIdx, config.groups.doubleRoundRobin);
        int rounds = static_cast<int>(max(north.size(), south.size()));
        for (int r = 0; r < rounds; ++r) {
            vector<pair<int, int>> matches;
            if (r < static_cast<int>(north.size())) {
                matches.insert(matches.end(), north[r].begin(), north[r].end());
            }
            if (r < static_cast<int>(south.size())) {
                matches.insert(matches.end(), south[r].begin(), south[r].end());
            }
            schedule.push_back(matches);
        }
        return;
    }

    vector<int> idx;
    idx.reserve(n);
    for (int i = 0; i < n; ++i) idx.push_back(i);
    schedule = buildRoundRobinSchedule(idx, true);
}

void Career::setActiveDivision(const string& id) {
    activeDivision = id;
    activeTeams = getDivisionTeams(id);
    leagueTable.clear();
    leagueTable.title = divisionDisplay(id);
    leagueTable.ruleId = id;
    for (auto* t : activeTeams) leagueTable.addTeam(t);
    leagueTable.sortTable();
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    buildRegionalGroups();
    buildSchedule();
}

static string encodeHeadToHead(const vector<HeadToHeadRecord>& records) {
    string out;
    for (size_t i = 0; i < records.size(); ++i) {
        if (i) out += ";";
        out += records[i].opponent + "=" + to_string(records[i].points);
    }
    return out;
}

static void decodeHeadToHead(const string& encoded, Team& team) {
    team.headToHead.clear();
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (token.empty()) continue;
        size_t eq = token.find('=');
        if (eq == string::npos) continue;
        string opponent = trim(token.substr(0, eq));
        string pointsStr = trim(token.substr(eq + 1));
        if (opponent.empty() || pointsStr.empty()) continue;
        try {
            team.headToHead.push_back({opponent, stoi(pointsStr)});
        } catch (...) {
        }
    }
}

static string encodeStringList(const vector<string>& items) {
    string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out += ";";
        out += items[i];
    }
    return out;
}

static vector<string> decodeStringList(const string& encoded) {
    vector<string> items;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) items.push_back(token);
    }
    return items;
}

static string encodeHistory(const vector<SeasonHistoryEntry>& entries) {
    string out;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i) out += "~";
        const auto& e = entries[i];
        out += to_string(e.season) + "^" + e.division + "^" + e.club + "^" + to_string(e.finish) + "^" +
               e.champion + "^" + e.promoted + "^" + e.relegated + "^" + e.note;
    }
    return out;
}

static vector<SeasonHistoryEntry> decodeHistory(const string& encoded) {
    vector<SeasonHistoryEntry> entries;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, '~')) {
        auto parts = splitByDelimiter(token, '^');
        if (parts.size() < 8) continue;
        SeasonHistoryEntry e;
        e.season = stoi(parts[0]);
        e.division = parts[1];
        e.club = parts[2];
        e.finish = stoi(parts[3]);
        e.champion = parts[4];
        e.promoted = parts[5];
        e.relegated = parts[6];
        e.note = parts[7];
        entries.push_back(e);
    }
    return entries;
}

static string encodePendingTransfers(const vector<PendingTransfer>& entries) {
    string out;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i) out += "~";
        const auto& e = entries[i];
        out += e.playerName + "^" + e.fromTeam + "^" + e.toTeam + "^" + to_string(e.effectiveSeason) + "^" +
               to_string(e.loanWeeks) + "^" + to_string(e.fee) + "^" + to_string(e.wage) + "^" +
               to_string(e.contractWeeks) + "^" + to_string(e.preContract ? 1 : 0) + "^" +
               to_string(e.loan ? 1 : 0) + "^" + e.promisedRole;
    }
    return out;
}

static vector<PendingTransfer> decodePendingTransfers(const string& encoded) {
    vector<PendingTransfer> entries;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, '~')) {
        auto parts = splitByDelimiter(token, '^');
        if (parts.size() < 8) continue;
        PendingTransfer e;
        e.playerName = parts[0];
        e.fromTeam = parts[1];
        e.toTeam = parts[2];
        e.effectiveSeason = stoi(parts[3]);
        e.loanWeeks = stoi(parts[4]);
        e.fee = stoll(parts[5]);
        if (parts.size() >= 10) {
            e.wage = stoll(parts[6]);
            e.contractWeeks = stoi(parts[7]);
            e.preContract = (parts[8] == "1");
            e.loan = (parts[9] == "1");
            e.promisedRole = (parts.size() > 10) ? parts[10] : "Sin promesa";
        } else {
            e.wage = 0;
            e.contractWeeks = 104;
            e.preContract = (parts[6] == "1");
            e.loan = (parts[7] == "1");
            e.promisedRole = "Sin promesa";
        }
        entries.push_back(e);
    }
    return entries;
}

static LeagueTable buildBoardTable(const vector<Team*>& teams, const string& ruleId) {
    LeagueTable table;
    table.ruleId = ruleId;
    for (auto* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

static vector<Team*> boardRelevantTeams(const Career& career) {
    vector<Team*> teams;
    if (!career.myTeam) return teams;
    if (career.usesGroupFormat()) {
        const auto& idx = [&career]() -> const vector<int>& {
            for (int i : career.groupNorthIdx) {
                if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) &&
                    career.activeTeams[i] == career.myTeam) {
                    return career.groupNorthIdx;
                }
            }
            return career.groupSouthIdx;
        }();
        for (int i : idx) {
            if (i >= 0 && i < static_cast<int>(career.activeTeams.size())) {
                teams.push_back(career.activeTeams[i]);
            }
        }
        if (!teams.empty()) return teams;
    }
    return career.activeTeams;
}

static int youthPlayersUsed(const Team& team) {
    int count = 0;
    for (const auto& player : team.players) {
        if (player.age <= 20 && player.matchesPlayed > 0) count++;
    }
    return count;
}

static bool promiseCurrentlyAtRisk(const Player& player, int currentWeek) {
    if (player.promisedRole == "Titular") {
        return player.startsThisSeason + 2 < max(2, currentWeek * 2 / 3);
    }
    if (player.promisedRole == "Rotacion") {
        return player.startsThisSeason + 1 < max(1, currentWeek / 3);
    }
    if (player.promisedRole == "Proyecto") {
        return player.age <= 22 && player.startsThisSeason < max(1, currentWeek / 4);
    }
    return false;
}

static int promisesAtRiskForBoard(const Team& team, int currentWeek) {
    int total = 0;
    for (const auto& player : team.players) {
        if (promiseCurrentlyAtRisk(player, currentWeek)) total++;
    }
    return total;
}

void Career::resetSeason() {
    for (auto& team : allTeams) {
        team.resetSeasonStats();
        team.morale = 50;
        for (auto& p : team.players) {
            p.injured = false;
            p.injuryWeeks = 0;
            p.injuryType.clear();
            p.fitness = p.stamina;
            p.yellowAccumulation = 0;
            p.seasonYellowCards = 0;
            p.seasonRedCards = 0;
            p.matchesSuspended = 0;
            p.goals = 0;
            p.assists = 0;
            p.matchesPlayed = 0;
            p.startsThisSeason = 0;
            p.wantsToLeave = false;
            p.happiness = clampInt(p.happiness + 5, 1, 99);
            p.currentForm = clampInt(44 + p.professionalism / 4 + randInt(-8, 8), 20, 88);
            ensurePlayerProfile(p, false);
        }
    }
    buildSchedule();
    initializeBoardObjectives();
    initializeSeasonCup();
    initializeDynamicObjective();
    lastMatchAnalysis.clear();
    scoutInbox.clear();
}

void Career::agePlayers() {
    for (auto& team : allTeams) {
        for (auto& player : team.players) {
            player.age++;
            if (player.age > 30) {
                player.skill = max(1, player.skill - 1);
                player.stamina = max(1, player.stamina - 1);
            }
            if (player.fitness > player.stamina) player.fitness = player.stamina;
            if (player.potential < player.skill) player.potential = player.skill;
            player.currentForm = clampInt((player.currentForm + 50) / 2 + randInt(-4, 4), 15, 90);
            ensurePlayerProfile(player, false);
        }
    }
}

Team* Career::findTeamByName(const string& name) {
    for (auto& team : allTeams) {
        if (team.name == name) return &team;
    }
    return nullptr;
}

const Team* Career::findTeamByName(const string& name) const {
    for (const auto& team : allTeams) {
        if (team.name == name) return &team;
    }
    return nullptr;
}

void Career::addNews(const string& item) {
    if (item.empty()) return;
    string entry = "T" + to_string(currentSeason) + "-F" + to_string(currentWeek) + ": " + item;
    newsFeed.push_back(entry);
    if (newsFeed.size() > 40) {
        newsFeed.erase(newsFeed.begin(), newsFeed.begin() + static_cast<long long>(newsFeed.size() - 40));
    }
}

void Career::executePendingTransfers() {
    for (size_t i = 0; i < pendingTransfers.size();) {
        PendingTransfer& move = pendingTransfers[i];
        if (move.effectiveSeason > currentSeason) {
            ++i;
            continue;
        }
        Team* from = findTeamByName(move.fromTeam);
        Team* to = findTeamByName(move.toTeam);
        if (!from || !to) {
            pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
            continue;
        }
        int playerIdx = -1;
        for (size_t p = 0; p < from->players.size(); ++p) {
            if (from->players[p].name == move.playerName) {
                playerIdx = static_cast<int>(p);
                break;
            }
        }
        if (playerIdx < 0) {
            pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
            continue;
        }
        Player player = from->players[playerIdx];
        if (move.preContract) {
            player.onLoan = false;
            player.parentClub.clear();
            player.loanWeeksRemaining = 0;
            if (move.wage > 0) player.wage = move.wage;
            if (move.contractWeeks > 0) player.contractWeeks = move.contractWeeks;
            player.releaseClause = max(player.value * 2, player.wage * 45);
            if (!move.promisedRole.empty()) {
                player.promisedRole = move.promisedRole;
                if (move.promisedRole == "Titular") player.desiredStarts = max(player.desiredStarts, 4);
                else if (move.promisedRole == "Rotacion") player.desiredStarts = max(player.desiredStarts, 2);
                else if (move.promisedRole == "Proyecto") player.desiredStarts = max(player.desiredStarts, 2);
            }
            from->players.erase(from->players.begin() + playerIdx);
            to->addPlayer(player);
            addNews(player.name + " se incorpora a " + to->name + " por precontrato.");
        }
        pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
    }
}

void Career::initializeSeasonCup() {
    cupRemainingTeams.clear();
    cupChampion.clear();
    cupRound = 0;
    cupActive = activeTeams.size() >= 4;
    if (!cupActive) return;
    for (auto* team : activeTeams) {
        if (team) cupRemainingTeams.push_back(team->name);
    }
}

void Career::initializeDynamicObjective() {
    if (!myTeam) return;
    int rank = currentCompetitiveRank();
    int field = max(1, currentCompetitiveFieldSize());
    boardMonthlyProgress = 0;
    boardMonthlyTarget = 0;
    boardMonthlyDeadlineWeek = min(static_cast<int>(schedule.size()), currentWeek + 3);
    if (boardMonthlyDeadlineWeek < currentWeek) boardMonthlyDeadlineWeek = currentWeek;

    int objectiveType = randInt(1, 4);
    if (objectiveType == 1) {
        boardMonthlyTarget = 6;
        boardMonthlyObjective = "Sumar al menos 6 puntos en 4 semanas";
    } else if (objectiveType == 2) {
        boardMonthlyTarget = 2;
        boardMonthlyObjective = "Dar 2 titularidades a sub-20";
    } else if (objectiveType == 3 && rank > 1) {
        boardMonthlyTarget = max(1, rank - 1);
        boardMonthlyProgress = (rank > 0) ? rank : field;
        boardMonthlyObjective = "Mejorar la posicion liguera antes de 4 semanas";
    } else {
        boardMonthlyTarget = static_cast<int>(max(100000LL, myTeam->budget * 80 / 100));
        boardMonthlyObjective = "Mantener presupuesto por sobre $" + to_string(boardMonthlyTarget);
        boardMonthlyProgress = static_cast<int>(min<long long>(myTeam->budget, 2000000000LL));
    }
}

void Career::updateDynamicObjectiveStatus() {
    if (!myTeam || boardMonthlyObjective.empty()) return;
    int youthStarts = 0;
    for (const auto& player : myTeam->players) {
        if (player.age <= 20) youthStarts += player.startsThisSeason;
    }
    if (boardMonthlyObjective.find("puntos") != string::npos) {
        // progress updated externally from weekly points gain
    } else if (boardMonthlyObjective.find("titularidades") != string::npos) {
        boardMonthlyProgress = youthStarts;
    } else if (boardMonthlyObjective.find("posicion") != string::npos) {
        boardMonthlyProgress = currentCompetitiveRank();
    } else if (boardMonthlyObjective.find("presupuesto") != string::npos) {
        boardMonthlyProgress = static_cast<int>(min<long long>(myTeam->budget, 2000000000LL));
    }

    if (currentWeek < boardMonthlyDeadlineWeek) return;
    bool success = false;
    if (boardMonthlyObjective.find("puntos") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("titularidades") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("posicion") != string::npos) {
        success = boardMonthlyProgress > 0 && boardMonthlyProgress <= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("presupuesto") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    }
    boardConfidence = clampInt(boardConfidence + (success ? 4 : -4), 0, 100);
    addNews(string("Objetivo mensual ") + (success ? "cumplido: " : "fallado: ") + boardMonthlyObjective);
    initializeDynamicObjective();
}

int Career::currentCompetitiveRank() const {
    if (!myTeam) return -1;
    vector<Team*> teams = boardRelevantTeams(*this);
    LeagueTable table = buildBoardTable(teams, activeDivision);
    for (size_t i = 0; i < table.teams.size(); ++i) {
        if (table.teams[i] == myTeam) return static_cast<int>(i) + 1;
    }
    return -1;
}

int Career::currentCompetitiveFieldSize() const {
    return static_cast<int>(boardRelevantTeams(*this).size());
}

void Career::initializeBoardObjectives() {
    if (!myTeam) return;
    vector<Team*> teams = boardRelevantTeams(*this);
    if (teams.empty()) return;
    vector<Team*> byValue = teams;
    sort(byValue.begin(), byValue.end(), [](Team* a, Team* b) {
        if (a->getSquadValue() != b->getSquadValue()) return a->getSquadValue() > b->getSquadValue();
        return a->name < b->name;
    });
    int valueRank = static_cast<int>(byValue.size());
    for (size_t i = 0; i < byValue.size(); ++i) {
        if (byValue[i] == myTeam) {
            valueRank = static_cast<int>(i) + 1;
            break;
        }
    }

    int fieldSize = static_cast<int>(teams.size());
    boardExpectedFinish = clampInt(valueRank + 1, 1, fieldSize);
    boardBudgetTarget = max(150000LL, myTeam->budget * 60 / 100);
    boardYouthTarget = (fieldSize >= 14) ? 2 : 1;
    boardConfidence = clampInt(55 + max(0, boardExpectedFinish - valueRank) * 4, 45, 75);
    boardWarningWeeks = 0;
}

void Career::updateBoardConfidence() {
    if (!myTeam || boardExpectedFinish <= 0) return;
    int rank = currentCompetitiveRank();
    int delta = 0;
    int prestige = teamPrestigeScore(*myTeam);
    if (rank > 0) {
        if (rank <= boardExpectedFinish) delta += 3;
        else delta -= min(5, rank - boardExpectedFinish);
        if (prestige >= 70 && rank > boardExpectedFinish) delta--;
    }
    if (myTeam->budget >= boardBudgetTarget) delta += 1;
    else delta -= 2;
    if (myTeam->budget < boardBudgetTarget * 8 / 10) delta -= 1;
    if (myTeam->debt > myTeam->sponsorWeekly * 16) delta -= 2;

    int youthUsed = youthPlayersUsed(*myTeam);
    if (youthUsed >= boardYouthTarget) delta += 1;
    else if (currentWeek > max(3, static_cast<int>(schedule.size()) / 4)) delta -= 1;

    if (myTeam->morale >= 65) delta += 1;
    else if (myTeam->morale <= 35) delta -= 1;
    if (myTeam->fanBase >= 55 && myTeam->morale <= 42) delta -= 1;

    int promiseRisk = promisesAtRiskForBoard(*myTeam, currentWeek);
    if (promiseRisk >= 2) delta -= min(3, promiseRisk);

    boardConfidence = clampInt(boardConfidence + delta, 0, 100);
    if (boardConfidence < 35) boardWarningWeeks++;
    else boardWarningWeeks = 0;
}

static string encodePlayerFields(const Player& p) {
    stringstream ss;
    ss << p.name << "|" << p.position << "|" << p.attack << "|" << p.defense << "|"
       << p.stamina << "|" << p.fitness << "|" << p.skill << "|" << p.potential << "|" << p.age << "|" << p.value << "|"
       << p.onLoan << "|" << p.parentClub << "|" << p.loanWeeksRemaining << "|"
       << p.injured << "|" << p.injuryType << "|" << p.injuryWeeks << "|" << p.goals << "|" << p.assists << "|"
       << p.matchesPlayed << "|" << p.lastTrainedSeason << "|" << p.lastTrainedWeek << "|"
       << p.wage << "|" << p.releaseClause << "|" << p.contractWeeks << "|" << p.injuryHistory << "|"
       << p.yellowAccumulation << "|" << p.seasonYellowCards << "|" << p.seasonRedCards << "|" << p.matchesSuspended
       << "|" << p.role << "|" << p.setPieceSkill << "|" << p.leadership << "|" << p.professionalism
       << "|" << p.ambition << "|" << p.happiness << "|" << p.chemistry << "|" << p.desiredStarts
       << "|" << p.startsThisSeason << "|" << p.wantsToLeave
       << "|" << p.developmentPlan << "|" << p.promisedRole << "|" << encodeStringList(p.traits)
       << "|" << p.preferredFoot << "|" << encodeStringList(p.secondaryPositions)
       << "|" << p.consistency << "|" << p.bigMatches << "|" << p.currentForm
       << "|" << p.tacticalDiscipline << "|" << p.versatility;
    return ss.str();
}

static Player decodePlayerFields(const vector<string>& pf) {
    Player p;
    size_t idx = 0;
    bool hasLoanState = pf.size() >= 30;
    if (idx < pf.size()) p.name = pf[idx++];
    if (idx < pf.size()) p.position = pf[idx++];
    if (idx < pf.size()) p.attack = stoi(pf[idx++]); else p.attack = 40;
    if (idx < pf.size()) p.defense = stoi(pf[idx++]); else p.defense = 40;
    if (idx < pf.size()) p.stamina = stoi(pf[idx++]); else p.stamina = 60;

    bool hasFitness = (pf.size() >= 16);
    if (hasFitness && idx < pf.size()) p.fitness = stoi(pf[idx++]);
    else p.fitness = p.stamina;

    if (idx < pf.size()) p.skill = stoi(pf[idx++]); else p.skill = (p.attack + p.defense) / 2;

    size_t remaining = (idx <= pf.size()) ? (pf.size() - idx) : 0;
    bool hasPotential = (remaining >= 11);
    if (hasPotential && idx < pf.size()) p.potential = stoi(pf[idx++]);
    else p.potential = clampInt(p.skill + randInt(0, 8), p.skill, 95);

    if (idx < pf.size()) p.age = stoi(pf[idx++]); else p.age = 24;
    if (idx < pf.size()) p.value = stoll(pf[idx++]); else p.value = static_cast<long long>(p.skill) * 10000;
    if (hasLoanState && idx < pf.size()) {
        string loanStr = toLower(pf[idx++]);
        p.onLoan = (loanStr == "1" || loanStr == "true");
    } else {
        p.onLoan = false;
    }
    if (hasLoanState && idx < pf.size()) p.parentClub = pf[idx++];
    else p.parentClub.clear();
    if (hasLoanState && idx < pf.size()) p.loanWeeksRemaining = stoi(pf[idx++]);
    else p.loanWeeksRemaining = 0;

    string injuredStr = (idx < pf.size()) ? toLower(pf[idx++]) : "0";
    p.injured = (injuredStr == "1" || injuredStr == "true");

    if (hasPotential && idx < pf.size()) p.injuryType = pf[idx++];
    else p.injuryType = p.injured ? "Leve" : "";
    if (idx < pf.size()) p.injuryWeeks = stoi(pf[idx++]); else p.injuryWeeks = 0;
    if (idx < pf.size()) p.goals = stoi(pf[idx++]); else p.goals = 0;
    if (idx < pf.size()) p.assists = stoi(pf[idx++]); else p.assists = 0;
    if (idx < pf.size()) p.matchesPlayed = stoi(pf[idx++]); else p.matchesPlayed = 0;
    if (idx + 1 < pf.size()) {
        p.lastTrainedSeason = stoi(pf[idx++]);
        p.lastTrainedWeek = stoi(pf[idx++]);
    } else {
        p.lastTrainedSeason = -1;
        p.lastTrainedWeek = -1;
    }

    if (idx < pf.size()) p.wage = stoll(pf[idx++]);
    else p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 800);
    if (idx < pf.size()) p.releaseClause = stoll(pf[idx++]);
    else p.releaseClause = max(50000LL, p.value * 2);
    if (idx < pf.size()) p.contractWeeks = stoi(pf[idx++]);
    else p.contractWeeks = randInt(52, 156);
    if (idx < pf.size()) p.injuryHistory = stoi(pf[idx++]);
    else p.injuryHistory = 0;
    if (idx < pf.size()) p.yellowAccumulation = stoi(pf[idx++]);
    else p.yellowAccumulation = 0;
    if (idx < pf.size()) p.seasonYellowCards = stoi(pf[idx++]);
    else p.seasonYellowCards = 0;
    if (idx < pf.size()) p.seasonRedCards = stoi(pf[idx++]);
    else p.seasonRedCards = 0;
    if (idx < pf.size()) p.matchesSuspended = stoi(pf[idx++]);
    else p.matchesSuspended = 0;
    if (idx < pf.size()) p.role = pf[idx++];
    else p.role = defaultRoleForPosition(p.position);
    if (idx < pf.size()) p.setPieceSkill = stoi(pf[idx++]);
    else p.setPieceSkill = clampInt(p.skill + randInt(-6, 6), 25, 99);
    if (idx < pf.size()) p.leadership = stoi(pf[idx++]);
    else p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    if (idx < pf.size()) p.professionalism = stoi(pf[idx++]);
    else p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    if (idx < pf.size()) p.ambition = stoi(pf[idx++]);
    else p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    if (idx < pf.size()) p.happiness = stoi(pf[idx++]);
    else p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    if (idx < pf.size()) p.chemistry = stoi(pf[idx++]);
    else p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    if (idx < pf.size()) p.desiredStarts = stoi(pf[idx++]);
    else p.desiredStarts = 1;
    if (idx < pf.size()) p.startsThisSeason = stoi(pf[idx++]);
    else p.startsThisSeason = 0;
    if (idx < pf.size()) {
        string wantsOut = toLower(pf[idx++]);
        p.wantsToLeave = (wantsOut == "1" || wantsOut == "true");
    } else {
        p.wantsToLeave = false;
    }
    if (idx < pf.size()) p.developmentPlan = pf[idx++];
    else p.developmentPlan = defaultDevelopmentPlanForPosition(p.position);
    if (idx < pf.size()) p.promisedRole = pf[idx++];
    else p.promisedRole = "Sin promesa";
    if (idx < pf.size()) p.traits = decodeStringList(pf[idx++]);
    if (idx < pf.size()) p.preferredFoot = pf[idx++];
    if (idx < pf.size()) p.secondaryPositions = decodeStringList(pf[idx++]);
    if (idx < pf.size()) p.consistency = stoi(pf[idx++]);
    else p.consistency = 0;
    if (idx < pf.size()) p.bigMatches = stoi(pf[idx++]);
    else p.bigMatches = 0;
    if (idx < pf.size()) p.currentForm = stoi(pf[idx++]);
    else p.currentForm = 0;
    if (idx < pf.size()) p.tacticalDiscipline = stoi(pf[idx++]);
    else p.tacticalDiscipline = 0;
    if (idx < pf.size()) p.versatility = stoi(pf[idx++]);
    else p.versatility = 0;
    ensurePlayerProfile(p, p.traits.empty());
    return p;
}

bool Career::saveCareer() {
    if (saveFile.rfind("saves/", 0) == 0 || saveFile.rfind("saves\\", 0) == 0) {
#ifdef _WIN32
        _mkdir("saves");
#else
        mkdir("saves", 0755);
#endif
    }
    ofstream file(saveFile);
    if (!file.is_open()) return false;

    file << "VERSION " << kCareerSaveVersion << "\n";
    file << "SEASON " << currentSeason << " WEEK " << currentWeek << "\n";
    file << "DIVISION " << activeDivision << "\n";
    file << "MYTEAM " << (myTeam ? myTeam->name : "") << "\n";
    file << "MANAGER " << managerName << "|" << managerReputation << "\n";
    file << "DYNOBJ " << boardMonthlyObjective << "|" << boardMonthlyTarget << "|" << boardMonthlyProgress
         << "|" << boardMonthlyDeadlineWeek << "\n";
    file << "ACHIEVEMENTS " << achievements.size() << "\n";
    for (const auto& ach : achievements) {
        file << "ACH " << ach << "\n";
    }
    file << "BOARD " << boardConfidence << "|" << boardExpectedFinish << "|" << boardBudgetTarget
         << "|" << boardYouthTarget << "|" << boardWarningWeeks << "\n";
    file << "NEWS " << encodeStringList(newsFeed) << "\n";
    file << "SCOUT " << encodeStringList(scoutInbox) << "\n";
    file << "SHORTLIST " << encodeStringList(scoutingShortlist) << "\n";
    file << "HISTORY " << encodeHistory(history) << "\n";
    file << "PENDING " << encodePendingTransfers(pendingTransfers) << "\n";
    file << "CUP " << (cupActive ? 1 : 0) << "|" << cupRound << "|" << encodeStringList(cupRemainingTeams)
         << "|" << cupChampion << "\n";
    file << "LASTMATCH " << lastMatchAnalysis << "\n";
    file << "TEAMS " << allTeams.size() << "\n";

    for (auto& team : allTeams) {
        ensureTeamIdentity(team);
        file << "TEAM " << team.name << "|" << team.division << "|" << team.tactics << "|" << team.formation
             << "|" << team.budget << "|" << team.morale << "|" << team.points << "|" << team.goalsFor << "|" << team.goalsAgainst
             << "|" << team.wins << "|" << team.draws << "|" << team.losses << "|" << team.trainingFocus
             << "|" << team.awayGoals << "|" << team.redCards << "|" << team.yellowCards << "|" << team.tiebreakerSeed
             << "|" << encodeHeadToHead(team.headToHead)
             << "|" << team.pressingIntensity << "|" << team.defensiveLine << "|" << team.tempo << "|" << team.width
             << "|" << team.markingStyle << "|" << encodeStringList(team.preferredXI) << "|" << encodeStringList(team.preferredBench)
             << "|" << team.captain << "|" << team.penaltyTaker << "|" << team.freeKickTaker << "|" << team.cornerTaker
             << "|" << team.rotationPolicy << "|" << team.assistantCoach << "|" << team.fitnessCoach << "|" << team.scoutingChief
             << "|" << team.youthCoach << "|" << team.medicalTeam << "|" << team.youthRegion << "|" << team.debt
             << "|" << team.sponsorWeekly << "|" << team.stadiumLevel << "|" << team.youthFacilityLevel
             << "|" << team.trainingFacilityLevel << "|" << team.fanBase << "|" << team.matchInstruction
             << "|" << teamPrestigeScore(team) << "|" << team.clubStyle << "|" << team.youthIdentity
             << "|" << team.primaryRival << "\n";
        file << "PLAYERS " << team.players.size() << "\n";
        for (const auto& p : team.players) {
            file << "PLAYER " << encodePlayerFields(p) << "\n";
        }
        file << "ENDTEAM\n";
    }
    return true;
}

bool Career::loadCareer() {
    string resolvedSave = saveFile;
    if (!pathExists(resolvedSave) && resolvedSave == "saves/career_save.txt" && pathExists("career_save.txt")) {
        resolvedSave = "career_save.txt";
    }
    ifstream file(resolvedSave);
    if (!file.is_open()) return false;

    string line;
    if (!getline(file, line)) return false;
    line = trim(line);

    int saveVersion = 1;
    if (line.rfind("VERSION ", 0) == 0) {
        saveVersion = stoi(trim(line.substr(8)));
        if (saveVersion > kCareerSaveVersion) return false;
        if (!getline(file, line)) return false;
        line = trim(line);
    }

    if (line.rfind("SEASON ", 0) == 0) {
        string token;
        stringstream ss(line);
        ss >> token >> currentSeason >> token >> currentWeek;

        string divisionLine;
        if (!getline(file, divisionLine)) return false;
        if (divisionLine.rfind("DIVISION ", 0) == 0) activeDivision = trim(divisionLine.substr(9));

        string myTeamLine;
        if (!getline(file, myTeamLine)) return false;
        string myTeamName;
        if (myTeamLine.rfind("MYTEAM ", 0) == 0) myTeamName = trim(myTeamLine.substr(7));

        string teamsLine;
        if (!getline(file, teamsLine)) return false;
        managerName = "Manager";
        managerReputation = 50;
        if (teamsLine.rfind("MANAGER ", 0) == 0) {
            auto managerFields = splitByDelimiter(trim(teamsLine.substr(8)), '|');
            if (!managerFields.empty()) managerName = managerFields[0];
            if (managerFields.size() > 1) managerReputation = stoi(managerFields[1]);
            if (!getline(file, teamsLine)) return false;
        }
        boardMonthlyObjective.clear();
        boardMonthlyTarget = 0;
        boardMonthlyProgress = 0;
        boardMonthlyDeadlineWeek = 0;
        if (teamsLine.rfind("DYNOBJ ", 0) == 0) {
            auto objectiveFields = splitByDelimiter(trim(teamsLine.substr(7)), '|');
            if (!objectiveFields.empty()) boardMonthlyObjective = objectiveFields[0];
            if (objectiveFields.size() > 1) boardMonthlyTarget = stoi(objectiveFields[1]);
            if (objectiveFields.size() > 2) boardMonthlyProgress = stoi(objectiveFields[2]);
            if (objectiveFields.size() > 3) boardMonthlyDeadlineWeek = stoi(objectiveFields[3]);
            if (!getline(file, teamsLine)) return false;
        }
        achievements.clear();
        if (teamsLine.rfind("ACHIEVEMENTS ", 0) == 0) {
            int achCount = stoi(trim(teamsLine.substr(13)));
            for (int i = 0; i < achCount; ++i) {
                if (!getline(file, line)) return false;
                if (line.rfind("ACH ", 0) != 0) return false;
                achievements.push_back(trim(line.substr(4)));
            }
            if (!getline(file, teamsLine)) return false;
        }
        boardConfidence = 60;
        boardExpectedFinish = 0;
        boardBudgetTarget = 0;
        boardYouthTarget = 1;
        boardWarningWeeks = 0;
        if (teamsLine.rfind("BOARD ", 0) == 0) {
            auto boardFields = splitByDelimiter(trim(teamsLine.substr(6)), '|');
            if (boardFields.size() > 0) boardConfidence = clampInt(stoi(boardFields[0]), 0, 100);
            if (boardFields.size() > 1) boardExpectedFinish = stoi(boardFields[1]);
            if (boardFields.size() > 2) boardBudgetTarget = stoll(boardFields[2]);
            if (boardFields.size() > 3) boardYouthTarget = stoi(boardFields[3]);
            if (boardFields.size() > 4) boardWarningWeeks = stoi(boardFields[4]);
            if (!getline(file, teamsLine)) return false;
        }
        newsFeed.clear();
        if (teamsLine.rfind("NEWS ", 0) == 0) {
            newsFeed = decodeStringList(trim(teamsLine.substr(5)));
            if (!getline(file, teamsLine)) return false;
        }
        scoutInbox.clear();
        if (teamsLine.rfind("SCOUT ", 0) == 0) {
            scoutInbox = decodeStringList(trim(teamsLine.substr(6)));
            if (!getline(file, teamsLine)) return false;
        }
        scoutingShortlist.clear();
        if (teamsLine.rfind("SHORTLIST ", 0) == 0) {
            scoutingShortlist = decodeStringList(trim(teamsLine.substr(10)));
            if (!getline(file, teamsLine)) return false;
        }
        history.clear();
        if (teamsLine.rfind("HISTORY ", 0) == 0) {
            history = decodeHistory(trim(teamsLine.substr(8)));
            if (!getline(file, teamsLine)) return false;
        }
        pendingTransfers.clear();
        if (teamsLine.rfind("PENDING ", 0) == 0) {
            pendingTransfers = decodePendingTransfers(trim(teamsLine.substr(8)));
            if (!getline(file, teamsLine)) return false;
        }
        cupActive = false;
        cupRound = 0;
        cupRemainingTeams.clear();
        cupChampion.clear();
        if (teamsLine.rfind("CUP ", 0) == 0) {
            auto cupFields = splitByDelimiter(trim(teamsLine.substr(4)), '|');
            if (!cupFields.empty()) cupActive = (cupFields[0] == "1");
            if (cupFields.size() > 1) cupRound = stoi(cupFields[1]);
            if (cupFields.size() > 2) cupRemainingTeams = decodeStringList(cupFields[2]);
            if (cupFields.size() > 3) cupChampion = cupFields[3];
            if (!getline(file, teamsLine)) return false;
        }
        lastMatchAnalysis.clear();
        if (teamsLine.rfind("LASTMATCH ", 0) == 0) {
            lastMatchAnalysis = trim(teamsLine.substr(10));
            if (!getline(file, teamsLine)) return false;
        }
        if (teamsLine.rfind("TEAMS ", 0) != 0) return false;
        int teamCount = stoi(trim(teamsLine.substr(6)));

        allTeams.clear();
        for (int i = 0; i < teamCount; ++i) {
            if (!getline(file, line)) return false;
            if (line.rfind("TEAM ", 0) != 0) return false;
            auto fields = splitByDelimiter(trim(line.substr(5)), '|');
            if (fields.size() < 11) return false;

            Team team(fields[0]);
            team.division = fields[1];
            team.tactics = fields[2];
            team.formation = fields[3];
            team.budget = stoll(fields[4]);
            size_t base = 5;
            if (fields.size() >= 12) {
                team.morale = clampInt(stoi(fields[5]), 0, 100);
                base = 6;
            } else {
                team.morale = 50;
            }
            team.points = stoi(fields[base + 0]);
            team.goalsFor = stoi(fields[base + 1]);
            team.goalsAgainst = stoi(fields[base + 2]);
            team.wins = stoi(fields[base + 3]);
            team.draws = stoi(fields[base + 4]);
            team.losses = stoi(fields[base + 5]);
            size_t tfIndex = base + 6;
            if (fields.size() > tfIndex) team.trainingFocus = fields[tfIndex];
            else team.trainingFocus = "Balanceado";
            size_t extra = tfIndex + 1;
            if (fields.size() > extra) team.awayGoals = stoi(fields[extra]);
            else team.awayGoals = 0;
            if (fields.size() > extra + 1) team.redCards = stoi(fields[extra + 1]);
            else team.redCards = 0;
            if (fields.size() > extra + 2) team.yellowCards = stoi(fields[extra + 2]);
            else team.yellowCards = 0;
            if (fields.size() > extra + 3) team.tiebreakerSeed = stoi(fields[extra + 3]);
            else team.tiebreakerSeed = randInt(0, 1000000);
            if (fields.size() > extra + 4) decodeHeadToHead(fields[extra + 4], team);
            else team.headToHead.clear();
            if (fields.size() > extra + 5) team.pressingIntensity = clampInt(stoi(fields[extra + 5]), 1, 5);
            else team.pressingIntensity = 3;
            if (fields.size() > extra + 6) team.defensiveLine = clampInt(stoi(fields[extra + 6]), 1, 5);
            else team.defensiveLine = 3;
            if (fields.size() > extra + 7) team.tempo = clampInt(stoi(fields[extra + 7]), 1, 5);
            else team.tempo = 3;
            if (fields.size() > extra + 8) team.width = clampInt(stoi(fields[extra + 8]), 1, 5);
            else team.width = 3;
            if (fields.size() > extra + 9) team.markingStyle = fields[extra + 9];
            else team.markingStyle = "Zonal";
            if (fields.size() > extra + 10) team.preferredXI = decodeStringList(fields[extra + 10]);
            else team.preferredXI.clear();
            if (fields.size() > extra + 11) team.preferredBench = decodeStringList(fields[extra + 11]);
            else team.preferredBench.clear();
            if (fields.size() > extra + 12) team.captain = fields[extra + 12];
            else team.captain.clear();
            if (fields.size() > extra + 13) team.penaltyTaker = fields[extra + 13];
            else team.penaltyTaker.clear();
            if (fields.size() > extra + 14) team.freeKickTaker = fields[extra + 14];
            else team.freeKickTaker.clear();
            if (fields.size() > extra + 15) team.cornerTaker = fields[extra + 15];
            else team.cornerTaker.clear();
            if (fields.size() > extra + 16) team.rotationPolicy = fields[extra + 16];
            else team.rotationPolicy = "Balanceado";
            if (fields.size() > extra + 17) team.assistantCoach = clampInt(stoi(fields[extra + 17]), 1, 99);
            else team.assistantCoach = 55;
            if (fields.size() > extra + 18) team.fitnessCoach = clampInt(stoi(fields[extra + 18]), 1, 99);
            else team.fitnessCoach = 55;
            if (fields.size() > extra + 19) team.scoutingChief = clampInt(stoi(fields[extra + 19]), 1, 99);
            else team.scoutingChief = 55;
            if (fields.size() > extra + 20) team.youthCoach = clampInt(stoi(fields[extra + 20]), 1, 99);
            else team.youthCoach = 55;
            if (fields.size() > extra + 21) team.medicalTeam = clampInt(stoi(fields[extra + 21]), 1, 99);
            else team.medicalTeam = 55;
            if (fields.size() > extra + 22) team.youthRegion = fields[extra + 22];
            else team.youthRegion = "Metropolitana";
            if (fields.size() > extra + 23) team.debt = stoll(fields[extra + 23]);
            else team.debt = 0;
            if (fields.size() > extra + 24) team.sponsorWeekly = stoll(fields[extra + 24]);
            else team.sponsorWeekly = 25000;
            if (fields.size() > extra + 25) team.stadiumLevel = stoi(fields[extra + 25]);
            else team.stadiumLevel = 1;
            if (fields.size() > extra + 26) team.youthFacilityLevel = stoi(fields[extra + 26]);
            else team.youthFacilityLevel = 1;
            if (fields.size() > extra + 27) team.trainingFacilityLevel = stoi(fields[extra + 27]);
            else team.trainingFacilityLevel = 1;
            if (fields.size() > extra + 28) team.fanBase = stoi(fields[extra + 28]);
            else team.fanBase = 12;
            if (fields.size() > extra + 29) team.matchInstruction = fields[extra + 29];
            else team.matchInstruction = "Equilibrado";
            if (fields.size() > extra + 30) team.clubPrestige = clampInt(stoi(fields[extra + 30]), 1, 99);
            else team.clubPrestige = 0;
            if (fields.size() > extra + 31) team.clubStyle = fields[extra + 31];
            else team.clubStyle.clear();
            if (fields.size() > extra + 32) team.youthIdentity = fields[extra + 32];
            else team.youthIdentity.clear();
            if (fields.size() > extra + 33) team.primaryRival = fields[extra + 33];
            else team.primaryRival.clear();
            if (fields.size() <= extra + 19) {
                team.preferredBench.clear();
                if (fields.size() > extra + 11) team.captain = fields[extra + 11];
                if (fields.size() > extra + 12) team.penaltyTaker = fields[extra + 12];
                if (fields.size() > extra + 13) team.rotationPolicy = fields[extra + 13];
                if (fields.size() > extra + 14) team.debt = stoll(fields[extra + 14]);
                if (fields.size() > extra + 15) team.sponsorWeekly = stoll(fields[extra + 15]);
                if (fields.size() > extra + 16) team.stadiumLevel = stoi(fields[extra + 16]);
                if (fields.size() > extra + 17) team.youthFacilityLevel = stoi(fields[extra + 17]);
                if (fields.size() > extra + 18) team.trainingFacilityLevel = stoi(fields[extra + 18]);
                if (fields.size() > extra + 19) team.fanBase = stoi(fields[extra + 19]);
                team.freeKickTaker = team.penaltyTaker;
                team.cornerTaker = team.penaltyTaker;
                team.assistantCoach = team.fitnessCoach = team.scoutingChief = team.youthCoach = team.medicalTeam = 55;
                team.youthRegion = "Metropolitana";
                team.matchInstruction = "Equilibrado";
            }
            ensureTeamIdentity(team);

            if (!getline(file, line)) return false;
            if (line.rfind("PLAYERS ", 0) != 0) return false;
            int playersCount = stoi(trim(line.substr(8)));
            for (int j = 0; j < playersCount; ++j) {
                if (!getline(file, line)) return false;
                if (line.rfind("PLAYER ", 0) != 0) return false;
                auto pf = splitByDelimiter(trim(line.substr(7)), '|');
                if (pf.size() < 13) continue;
                team.addPlayer(decodePlayerFields(pf));
            }
            if (!getline(file, line)) return false;
            if (trim(line) != "ENDTEAM") return false;

            allTeams.push_back(std::move(team));
        }

        initialized = true;
        if (activeDivision.empty() && !allTeams.empty()) activeDivision = allTeams.front().division;
        setActiveDivision(activeDivision);

        myTeam = nullptr;
        for (auto& team : allTeams) {
            if (team.name == myTeamName) {
                myTeam = &team;
                break;
            }
        }
        if (boardExpectedFinish <= 0) initializeBoardObjectives();
        if (boardMonthlyObjective.empty()) initializeDynamicObjective();
        if (!cupActive && !activeTeams.empty()) initializeSeasonCup();
        return true;
    }

    file.clear();
    file.seekg(0);
    int season = 1;
    int week = 1;
    if (!(file >> season >> week)) return false;
    currentSeason = season;
    currentWeek = week;
    managerName = "Manager";
    managerReputation = 50;
    newsFeed.clear();
    scoutInbox.clear();
    scoutingShortlist.clear();
    history.clear();
    pendingTransfers.clear();
    boardMonthlyObjective.clear();
    boardMonthlyTarget = 0;
    boardMonthlyProgress = 0;
    boardMonthlyDeadlineWeek = 0;
    cupActive = false;
    cupRound = 0;
    cupRemainingTeams.clear();
    cupChampion.clear();
    lastMatchAnalysis.clear();
    string teamName;
    getline(file, teamName);
    getline(file, teamName);

    initializeLeague(true);
    myTeam = nullptr;
    for (auto& team : allTeams) {
        if (team.name == teamName) {
            myTeam = &team;
            break;
        }
    }

    if (myTeam) {
        myTeam->players.clear();
        string line2;
        getline(file, line2);
        while (getline(file, line2)) {
            if (line2.empty()) continue;
            stringstream ss(line2);
            string token;
            Player p;
            getline(ss, p.name, ',');
            getline(ss, p.position, ',');
            try {
                getline(ss, token, ','); p.attack = stoi(token);
                getline(ss, token, ','); p.defense = stoi(token);
                getline(ss, token, ','); p.stamina = stoi(token);
                p.fitness = p.stamina;
                getline(ss, token, ','); p.skill = stoi(token);
                p.potential = clampInt(p.skill + randInt(0, 6), p.skill, 95);
                getline(ss, token, ','); p.age = stoi(token);
                getline(ss, token, ','); p.value = stoll(token);
                p.onLoan = false;
                p.parentClub.clear();
                p.loanWeeksRemaining = 0;
                string injuredStr;
                getline(ss, injuredStr, ',');
                injuredStr = toLower(trim(injuredStr));
                p.injured = (injuredStr == "1" || injuredStr == "true");
                p.injuryType = p.injured ? "Leve" : "";
                getline(ss, token, ','); p.injuryWeeks = stoi(token);
                getline(ss, token, ','); p.goals = stoi(token);
                getline(ss, token, ','); p.assists = stoi(token);
                getline(ss, token, ','); p.matchesPlayed = stoi(token);
                p.lastTrainedSeason = -1;
                p.lastTrainedWeek = -1;
                p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 800);
                p.releaseClause = max(50000LL, p.value * 2);
                p.setPieceSkill = clampInt(p.skill + randInt(-6, 6), 25, 99);
                p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
                p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
                p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
                p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
                p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
                p.desiredStarts = 1;
                p.startsThisSeason = 0;
                p.wantsToLeave = false;
                p.contractWeeks = randInt(52, 156);
                p.injuryHistory = 0;
                p.yellowAccumulation = 0;
                p.seasonYellowCards = 0;
                p.seasonRedCards = 0;
                p.matchesSuspended = 0;
                ensurePlayerProfile(p, true);
                myTeam->addPlayer(p);
            } catch (...) {
            }
        }
    }

    if (myTeam) activeDivision = myTeam->division;
    else if (!divisions.empty()) activeDivision = divisions.front().id;
    if (!activeDivision.empty()) setActiveDivision(activeDivision);
    initializeBoardObjectives();
    initializeSeasonCup();
    initializeDynamicObjective();
    return true;
}
